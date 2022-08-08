// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2022 Oracle and/or its affiliates.
 */

/*\
 * [Description]
 * Stress the VMM and soft-offline code by spawning N threads which
 * allocate memory pages and soft-offline them repeatedly.
 * Control that soft-offlined pages get correctly replaced: with the
 * same content and without SIGBUS generation when accessed.
 * Could be used for example as a regression test-case for the
 * poisoned soft-offlined pages case fixed by upstream commit
 * d4ae9916ea29 (mm: soft-offline: close the race against page allocation)
 */

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/klog.h>
#include <time.h>

#include "tst_test.h"
#include "tst_safe_pthread.h"
#include "tst_safe_clocks.h"
#include "tst_safe_stdio.h"
#include "lapi/mmap.h"

#define NUM_THREADS	60	/* Number of threads to create */
#define NUM_LOOPS	 3	/* Number of loops per-thread */
#define NUM_PAGES	 5	/* Max number of allocated pages for each loop */

/* needed module to online back pages */
#define HW_MODULE	"hwpoison_inject"

static pthread_t *thread_id;		/* ptr to an array of spawned threads */
static long PAGESIZE;
static char beginningTag[BUFSIZ];	/* kmsg tag indicating our test start */
static int hwpoison_probe = 0;		/* do we have to probe hwpoison_inject? */

static void my_yield(void)
{
	static const struct timespec t0 = { 0, 0 };
	nanosleep(&t0, NULL);
}

/* a SIGBUS received is a confirmation of test failure */
static void sigbus_handler(int signum) {
	tst_res(TFAIL, "SIGBUS Received");
	_exit(signum);
}

/*
 * allocate_offline() - Allocate and offline test called per-thread
 *
 * This function does the allocation and offline by mmapping an
 * anonymous page and offlining it.
 *
 * Return:
 *  0: success
 *  1: failure
 */
static int allocate_offline(int tnum)
{
	int loop;
	const int MAXPTRS = NUM_PAGES;

	for (loop = 0; loop < NUM_LOOPS; loop++) {
		long *ptrs[MAXPTRS];
		int num_alloc;
		int i;

		/* loop terminates in one of two ways:
		 * 1. after MAXPTRS iterations
		 * 2. if page alloc fails
		 */
		for (num_alloc = 0; num_alloc < MAXPTRS; num_alloc++) {

			/* alloc the next page - the problem is more rapidly reproduced with MAP_SHARED */
			ptrs[num_alloc] = mmap(NULL, PAGESIZE, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
			/* terminate loop if alloc fails */
			if (!ptrs[num_alloc])
				break;
			/* write a uniq content */
			ptrs[num_alloc][0] = num_alloc;
			ptrs[num_alloc][1] = tnum;

			/* soft-offline requested */
			if (madvise(ptrs[num_alloc], PAGESIZE, MADV_SOFT_OFFLINE) == -1) {
				if (errno != EINVAL && errno != EBUSY)
					tst_res(TFAIL | TERRNO, "madvise failed");
				if (errno == EINVAL)
					tst_res(TCONF, "madvise() didn't support MADV_SOFT_OFFLINE");
				return errno;
			}
		}

		/* verify if the offlined pages content has changed */
		for (i = 0; i < num_alloc; i++) {
			if (ptrs[i][0] != i || ptrs[i][1] != tnum) {
				tst_res(TFAIL,
					"pid[%d] tnum[%d]: fail: bad sentinel value\n",
					getpid(), tnum);
				return 1;
			}
			munmap(ptrs[i], PAGESIZE);
		}

		my_yield();
		if (!tst_remaining_runtime()) {
			tst_res(TINFO, "Thread [%d]: Test runtime is over, exiting", tnum);
			break;
		}
	}

	/* Success! */
	return 0;
}

static void *alloc_mem(void *threadnum)
{
	int err;
	int tnum = (int)(uintptr_t)threadnum;

	/* waiting for other threads starting */
	TST_CHECKPOINT_WAIT(0);

	err = allocate_offline(tnum);
	tst_res(TINFO,
		"Thread [%d]: allocate_offline() returned %d, %s.  Thread exiting",
		tnum, err, (err ? "failed" : "succeeded"));
	return (void *)(uintptr_t) (err ? -1 : 0);
}

static void stress_alloc_offl(void)
{
	struct sigaction my_sigaction;
	int thread_index;
	int thread_failure = 0;

	/* SIGBUS is a failure criteria */
	my_sigaction.sa_handler = sigbus_handler;
	if (sigaction(SIGBUS, &my_sigaction, NULL) == -1)
		tst_res(TFAIL | TERRNO, "Signal handler attach failed");

	/* create all threads */
	for (thread_index = 0; thread_index < NUM_THREADS; thread_index++) {
		SAFE_PTHREAD_CREATE(&thread_id[thread_index], NULL, alloc_mem,
				    (void *)(uintptr_t)thread_index);
	}

	/* wake up all threads */
	TST_CHECKPOINT_WAKE2(0, NUM_THREADS);

	/* wait for all threads to finish */
	for (thread_index = 0; thread_index < NUM_THREADS; thread_index++) {
		void *status;

		SAFE_PTHREAD_JOIN(thread_id[thread_index], &status);
		if ((intptr_t)status != 0) {
			tst_res(TFAIL, "thread [%d] - exited with errors",
				thread_index);
			thread_failure++;
		}
	}

	if (thread_failure == 0)
		tst_res(TPASS, "soft-offline/alloc stress test finished successfully");
}

/*
 * ------------
 * Cleanup code:
 * The idea is to retrieve all the pfn numbers that have been soft-offined
 * (generating a "Soft offlining pfn 0x..." message in the kernel ring buffer)
 * by the current test (since a "beginningTag" message we write when starting).
 * And to put these pages back online by writing the pfn number to the
 * <debugfs>/hwpoison/unpoison-pfn special file.
 * ------------
 */
#define OFFLINE_PATTERN "Soft offlining pfn 0x"
#define OFFLINE_PATTERN_LEN sizeof(OFFLINE_PATTERN)

/* return the pfn if the kmsg msg is a soft-offline indication*/
static unsigned long parse_kmsg_soft_offlined_pfn(char *line, ssize_t len)
{
	char *pos;
	unsigned long addr = 0UL;

	pos = strstr(line, OFFLINE_PATTERN);
	if (pos == NULL)
		return 0UL;

	pos += OFFLINE_PATTERN_LEN-1;
	if (pos > (line + len))
		return 0UL;

	addr = strtoul(pos, NULL, 16);
	if ((addr == ULONG_MAX) && (errno == ERANGE))
		return 0UL;

	return addr;
}

/* return the pfns seen in kernel message log */
static int populate_from_klog(char* beginTag, unsigned long *pfns, int max)
{
	int found = 0, fd, beginningTag_found = 0;
	ssize_t sz;
	unsigned long pfn;
	char buf[BUFSIZ];

	fd = SAFE_OPEN("/dev/kmsg", O_RDONLY|O_NONBLOCK);

	while (found < max) {
		sz = read(fd, buf, sizeof(buf));
		/* kmsg returns EPIPE if record was modified while reading */
		if (sz < 0 && errno == EPIPE)
			continue;
		if (sz <= 0)
			break;
		if (!beginningTag_found) {
			if (strstr(buf, beginTag))
				beginningTag_found = 1;
			continue;
		}
		pfn = parse_kmsg_soft_offlined_pfn(buf, sz);
		if (pfn)
			pfns[found++] = pfn;
	}
	SAFE_CLOSE(fd);
	return found;
}

/*
 * Read the given file to search for the key.
 * If a valuePtr is given, copy the remaining of the line right
 * after the found key to the given location.
 * Return 1 if the key is found.
 */
static int find_in_file(char *path, char *key, char *valuePtr)
{
	char line[4096];
	char *pos = NULL;
	int found = 0;
	FILE *file = SAFE_FOPEN(path, "r");
	while (fgets(line, sizeof(line), file)) {
		pos = strstr(line, key);
		if (pos) {
			found = 1;
			if (valuePtr)
				strncpy(valuePtr, pos + strlen(key),
					line + strlen(line) - pos);
			break;
		}
	}
	SAFE_FCLOSE(file);
	return found;
}

static void unpoison_this_pfn(unsigned long pfn, int fd)
{
	char pfn_str[19]; /* unsigned long to ascii: 0x...\0 */

	snprintf(pfn_str, sizeof(pfn_str), "0x%lx", pfn);
	SAFE_WRITE(0, fd, pfn_str, strlen(pfn_str));
}

/* Find and open the <debugfs>/hwpoison/unpoison-pfn special file */
static int open_unpoison_pfn(void)
{
	char *added_file_path = "/hwpoison/unpoison-pfn";
	const char *const cmd_modprobe[] = {"modprobe", HW_MODULE, NULL};
	char debugfs_fp[4096];

	if (!find_in_file("/proc/modules", HW_MODULE, NULL))
		hwpoison_probe = 1;

	/* probe hwpoison only if it isn't already there */
	if (hwpoison_probe)
		SAFE_CMD(cmd_modprobe, NULL, NULL);

	/* debugfs mount point */
	if (find_in_file("/etc/mtab", "debugfs ", debugfs_fp) == 0)
		return -1;
	strcpy(strstr(debugfs_fp, " "), added_file_path);

	return (SAFE_OPEN(debugfs_fp, O_WRONLY));
}

/*
 * Get all the Offlined PFNs indicated in the dmesg output
 * starting after the given beginning tag, and request a debugfs
 * hwpoison/unpoison-pfn for each of them.
 */
static void unpoison_pfn(char* beginTag)
{

	/* maximum of pages to deal with */
	const int MAXPFN = NUM_THREADS*NUM_LOOPS*NUM_PAGES;
	unsigned long pfns[MAXPFN];
	const char *const cmd_rmmod[] = {"rmmod", HW_MODULE, NULL};
	int found_pfns, fd;

	fd = open_unpoison_pfn();
	if (fd >= 0) {
		found_pfns = populate_from_klog(beginTag, pfns, MAXPFN);

		tst_res(TINFO,"Restore %d Soft-offlined pages", found_pfns);
		/* unpoison in reverse order */
		while (found_pfns-- > 0)
			unpoison_this_pfn(pfns[found_pfns], fd);

		SAFE_CLOSE(fd);
	}
	/* remove hwpoison only if we probed it */
	if (hwpoison_probe)
		SAFE_CMD(cmd_rmmod, NULL, NULL);
}

/*
 * Create and write a beginning tag to the kernel buffer to be used on cleanup
 * when trying to restore the soft-offlined pages of our test run.
 */
static void write_beginningTag_to_kmsg(void)
{
	int fd;

	fd = SAFE_OPEN("/dev/kmsg", O_WRONLY);
	if (fd < 0) {
		tst_res(TCONF | TERRNO,"/dev/kmsg not available for writing");
		return;
	}
	snprintf(beginningTag, sizeof(beginningTag),
		 "Soft-offlining pages test starting (pid: %ld)",
		 (long)getpid());
	SAFE_WRITE(1, fd, beginningTag, strlen(beginningTag));
	SAFE_CLOSE(fd);
}

static void setup(void)
{
	thread_id = SAFE_MALLOC(sizeof(pthread_t) * NUM_THREADS);
	PAGESIZE = sysconf(_SC_PAGESIZE);
	write_beginningTag_to_kmsg();
}

static void cleanup(void)
{
	if (thread_id) {
		free(thread_id);
		thread_id = NULL;
	}
	unpoison_pfn(beginningTag);
}

static struct tst_test test = {
	.min_kver = "2.6.33",
	.needs_root = 1,
	.needs_drivers = (const char *const []) {
		HW_MODULE,
		NULL
	},
	.needs_cmds = (const char *[]) {
		"modprobe",
		"rmmod",
		NULL
	},
	.max_runtime = 300,
	.needs_checkpoints = 1,
	.setup = setup,
	.cleanup = cleanup,
	.test_all = stress_alloc_offl,
};
