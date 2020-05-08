// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2015  Yi Zhang <wetpzy@gmail.com>
 *                     Li Wang <liwang@redhat.com>
 *
 * DESCRIPTION:
 *
 *   It is a regression test for commit:
 *   http://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/
 *   commit/?id=13d60f4
 *
 *   The implementation of futex doesn't produce unique keys for futexes
 *   in shared huge pages, so threads waiting on different futexes may
 *   end up on the same wait list. This results in incorrect threads being
 *   woken by FUTEX_WAKE.
 *
 *   Needs to be run as root unless there are already enough huge pages available.
 *   In the fail case, which happens in the CentOS-6.6 kernel (2.6.32-504.8.1),
 *   the tests hangs until it times out after a 30-second wait.
 *
 */

#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <sys/time.h>
#include <string.h>

#include "futextest.h"
#include "futex_utils.h"
#include "lapi/mmap.h"
#include "lapi/abisize.h"
#include "tst_safe_stdio.h"

#define PATH_MEMINFO "/proc/meminfo"
#define PATH_NR_HUGEPAGES "/proc/sys/vm/nr_hugepages"
#define PATH_HUGEPAGES	"/sys/kernel/mm/hugepages/"

static futex_t *futex1, *futex2;

static struct tst_ts to;

static long orig_hugepages;

static struct test_variants {
	enum futex_fn_type fntype;
	enum tst_ts_type tstype;
	char *desc;
} variants[] = {
#if defined(TST_ABI32)
	{ .fntype = FUTEX_FN_FUTEX, .tstype = TST_KERN_OLD_TIMESPEC, .desc = "syscall with kernel spec32"},
#endif

#if defined(TST_ABI64)
	{ .fntype = FUTEX_FN_FUTEX, .tstype = TST_KERN_TIMESPEC, .desc = "syscall with kernel spec64"},
#endif

#if (__NR_futex_time64 != __LTP__NR_INVALID_SYSCALL)
	{ .fntype = FUTEX_FN_FUTEX64, .tstype = TST_KERN_TIMESPEC, .desc = "syscall time64 with kernel spec64"},
#endif
};

static void setup(void)
{
	struct test_variants *tv = &variants[tst_variant];

	tst_res(TINFO, "Testing variant: %s", tv->desc);

	to.type = tv->tstype;
	tst_ts_set_sec(&to, 30);
	tst_ts_set_nsec(&to, 0);

	if (access(PATH_HUGEPAGES, F_OK))
		tst_brk(TCONF, "Huge page is not supported.");

	SAFE_FILE_SCANF(PATH_NR_HUGEPAGES, "%ld", &orig_hugepages);

	if (orig_hugepages <= 0)
		SAFE_FILE_PRINTF(PATH_NR_HUGEPAGES, "%d", 1);
}

static void cleanup(void)
{
	if (orig_hugepages <= 0)
		SAFE_FILE_PRINTF(PATH_NR_HUGEPAGES, "%ld", orig_hugepages);
}

static int read_hugepagesize(void)
{
	FILE *fp;
	char line[BUFSIZ], buf[BUFSIZ];
	int val;

	fp = SAFE_FOPEN(PATH_MEMINFO, "r");
	while (fgets(line, BUFSIZ, fp) != NULL) {
		if (sscanf(line, "%64s %d", buf, &val) == 2)
			if (strcmp(buf, "Hugepagesize:") == 0) {
				SAFE_FCLOSE(fp);
				return 1024 * val;
			}
	}

	SAFE_FCLOSE(fp);
	tst_res(TFAIL, "can't find \"%s\" in %s", "Hugepagesize:",
		PATH_MEMINFO);
	return 0;
}

static void *wait_thread1(void *arg LTP_ATTRIBUTE_UNUSED)
{
	struct test_variants *tv = &variants[tst_variant];

	futex_wait(tv->fntype, futex1, *futex1, &to, 0);

	return NULL;
}

static void *wait_thread2(void *arg LTP_ATTRIBUTE_UNUSED)
{
	struct test_variants *tv = &variants[tst_variant];
	int res;

	res = futex_wait(tv->fntype, futex2, *futex2, &to, 0);
	if (!res)
		tst_res(TPASS, "Hi hydra, thread2 awake!");
	else
		tst_res(TFAIL | TTERRNO, "Bug: wait_thread2 did not wake after 30 secs.");

	return NULL;
}

static void wakeup_thread2(void)
{
	struct test_variants *tv = &variants[tst_variant];
	void *addr;
	int hpsz, pgsz, res;
	pthread_t th1, th2;

	hpsz = read_hugepagesize();
	tst_res(TINFO, "Hugepagesize %i", hpsz);

	/*allocate some shared memory*/
	addr = mmap(NULL, hpsz, PROT_WRITE | PROT_READ,
	            MAP_SHARED | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);

	if (addr == MAP_FAILED) {
		if (errno == ENOMEM)
			tst_res(TCONF, "Cannot allocate hugepage, memory too fragmented?");

		tst_res(TFAIL | TERRNO, "Cannot allocate hugepage");
		return;
	}

	pgsz = getpagesize();

	/*apply the first subpage to futex1*/
	futex1 = addr;
	*futex1 = 0;
	/*apply the second subpage to futex2*/
	futex2 = (futex_t *)((char *)addr + pgsz);
	*futex2 = 0;

	/*thread1 block on futex1 first,then thread2 block on futex2*/
	res = pthread_create(&th1, NULL, wait_thread1, NULL);
	if (res) {
		tst_res(TFAIL | TTERRNO, "pthread_create() failed");
		return;
	}

	res = pthread_create(&th2, NULL, wait_thread2, NULL);
	if (res) {
		tst_res(TFAIL | TTERRNO, "pthread_create() failed");
		return;
	}

	while (wait_for_threads(2))
		usleep(1000);

	futex_wake(tv->fntype, futex2, 1, 0);

	res = pthread_join(th2, NULL);
	if (res) {
		tst_res(TFAIL | TTERRNO, "pthread_join() failed");
		return;
	}

	futex_wake(tv->fntype, futex1, 1, 0);

	res = pthread_join(th1, NULL);
	if (res)
		tst_res(TFAIL | TTERRNO, "pthread_join() failed");

	SAFE_MUNMAP(addr, hpsz);
}

static void run(void)
{
	wakeup_thread2();
}

static struct tst_test test = {
	.setup = setup,
	.cleanup = cleanup,
	.test_all = run,
	.test_variants = ARRAY_SIZE(variants),
	.needs_root = 1,
	.min_kver = "2.6.32",
	.needs_tmpdir = 1,
};
