// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2005-2006 IBM Corporation
 * Author: David Gibson & Adam Litke
 */

/*\
 * [Description]
 *
 * This test is designed to detect a kernel allocation race introduced
 * with hugepage demand-faulting.  The problem is that no lock is held
 * between allocating a hugepage and instantiating it in the
 * pagetables or page cache index.  In between the two, the (huge)
 * page is cleared, so there's substantial time.  Thus two processes
 * can race instantiating the (same) last available hugepage - one
 * will fail on the allocation, and thus cause an OOM fault even
 * though the page it actually wants is being instantiated by the
 * other racing process.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include "tst_safe_pthread.h"
#include "hugetlb.h"
#define SYSFS_CPU_ONLINE_FMT	"/sys/devices/system/cpu/cpu%d/online"
#define MNTPOINT "hugetlbfs/"
unsigned long totpages;
static long hpage_size;
static char *str_op;
static int child1, child2, race_type, fd_sync, test_flag;
static pthread_t thread1, thread2;
static void *p_sync, *q_sync;

struct racer_info {
	void *p; /* instantiation address */
	int cpu;
	volatile int *mytrigger;
	volatile int *othertrigger;
	int status;
};

static int one_racer(void *p, int cpu,
		     volatile int *mytrigger, volatile int *othertrigger)
{
	volatile int *pi = p;
	cpu_set_t *cpuset;
	size_t mask_size;
	int err;

	// Allocate CPU mask dynamically
	cpuset = CPU_ALLOC(cpu + 1);
	if (!cpuset)
		tst_brk(TBROK | TERRNO, "CPU_ALLOC() failed");
	// Get the required size for the allocated CPU set
	mask_size = CPU_ALLOC_SIZE(cpu + 1);

	/* Split onto different CPUs to encourage the race */
	CPU_ZERO_S(mask_size, cpuset);
	CPU_SET_S(cpu, mask_size, cpuset);
	// Set CPU affinity using the allocated mask size
	err = sched_setaffinity(getpid(), mask_size, cpuset);
	if (err == -1)
		tst_brk(TBROK | TERRNO, "sched_setaffinity() failed");
	/* Ready */
	*mytrigger = 1;
	/* Wait for the other trigger to be set */
	while (!*othertrigger)
		;
	/* Set the shared value */
	*pi = 1;
	// Free the dynamically allocated CPU set
	CPU_FREE(cpuset);
	return 0;
}

static void proc_racer(void *p, int cpu,
		       volatile int *mytrigger, volatile int *othertrigger)
{
	exit(one_racer(p, cpu, mytrigger, othertrigger));
}

static void *thread_racer(void *info)
{
	struct racer_info *ri = info;

	ri->status = one_racer(ri->p, ri->cpu, ri->mytrigger, ri->othertrigger);
	return ri;
}

void check_online_cpus(int online_cpus[], int nr_cpus_needed)
{
	cpu_set_t cpuset;
	int total_cpus, cpu_idx, i;
	// Initialize the CPU set
	CPU_ZERO(&cpuset);
	// Get the total number of configured CPUs
	total_cpus = get_nprocs_conf();
	// Get the CPU affinity mask of the calling process
	if (sched_getaffinity(0, sizeof(cpu_set_t), &cpuset) == -1)
		tst_brk(TBROK | TERRNO, "sched_getaffinity() failed");

	// Check if there are enough online CPUs
	if (CPU_COUNT(&cpuset) < nr_cpus_needed)
		tst_brk(TBROK | TERRNO, "At least %d online CPUs are required ", nr_cpus_needed);

	cpu_idx = 0;
	// Find the first `nr_cpus_needed` CPUs in the affinity mask
	for (i = 0; i < total_cpus && cpu_idx < nr_cpus_needed; i++) {
		if (CPU_ISSET(i, &cpuset))
			online_cpus[cpu_idx++] = i;
	}
	if (cpu_idx < nr_cpus_needed)
		tst_brk(TBROK | TERRNO, "Unable to find enough online CPUs");
}

static void run_race(void *syncarea, int race_type)
{
	volatile int *trigger1, *trigger2;
	int fd;
	void *p, *tret1, *tret2;
	int status1, status2;
	int online_cpus[2];

	check_online_cpus(online_cpus, 2);
	memset(syncarea, 0, sizeof(*trigger1) + sizeof(*trigger2));
	trigger1 = syncarea;
	trigger2 = trigger1 + 1;

	/* Get a new file for the final page */
	fd = tst_creat_unlinked(MNTPOINT, 0);
	tst_res(TINFO, "Mapping final page.. ");
	p = SAFE_MMAP(NULL, hpage_size, PROT_READ|PROT_WRITE, race_type, fd, 0);
	if (race_type == MAP_SHARED) {
		child1 = SAFE_FORK();
		if (child1 == 0)
			proc_racer(p, online_cpus[0], trigger1, trigger2);

		child2 = SAFE_FORK();
		if (child2 == 0)
			proc_racer(p, online_cpus[1], trigger2, trigger1);

		/* wait() calls */
		SAFE_WAITPID(child1, &status1, 0);
		tst_res(TINFO, "Child 1 status: %x\n", status1);


		SAFE_WAITPID(child2, &status2, 0);
		tst_res(TINFO, "Child 2 status: %x\n", status2);

		if (WIFSIGNALED(status1))
			tst_res(TFAIL, "Child 1 killed by signal %s",
			strsignal(WTERMSIG(status1)));
		if (WIFSIGNALED(status2))
			tst_res(TFAIL, "Child 2 killed by signal %s",
			strsignal(WTERMSIG(status2)));

	} else {
		struct racer_info ri1 = {
			.p = p,
			.cpu = online_cpus[0],
			.mytrigger = trigger1,
			.othertrigger = trigger2,
			.status = -1,
		};
		struct racer_info ri2 = {
			.p = p,
			.cpu = online_cpus[1],
			.mytrigger = trigger2,
			.othertrigger = trigger1,
			.status = -1,
		};
		SAFE_PTHREAD_CREATE(&thread1, NULL, thread_racer, &ri1);
		SAFE_PTHREAD_CREATE(&thread2, NULL, thread_racer, &ri2);
		SAFE_PTHREAD_JOIN(thread1, &tret1);
		if (tret1 != &ri1) {
			test_flag = -1;
			tst_res(TFAIL, "Thread 1 returned %p not %p, killed?\n",
			     tret1, &ri1);
		}
		SAFE_PTHREAD_JOIN(thread2, &tret2);
		if (tret2 != &ri2) {
			test_flag = -1;
			tst_res(TFAIL, "Thread 2 returned %p not %p, killed?\n",
			     tret2, &ri2);
		}
		status1 = ri1.status;
		status2 = ri2.status;
	}

	if (status1 != 0) {
		test_flag = -1;
		tst_res(TFAIL, "Racer 1 terminated with code %d", status1);
	}

	if (status2 != 0) {
		test_flag = -1;
		tst_res(TFAIL, "Racer 2 terminated with code %d", status2);
	}
	if (test_flag != -1)
		test_flag = 0;

	if (fd >= 0)
		SAFE_CLOSE(fd);
}


static void run_test(void)
{
	unsigned long i;

	/* Allocate all save one of the pages up front */
	tst_res(TINFO, "instantiating.. ");
	for (i = 0; i < (totpages - 1); i++)
		memset(p_sync + (i * hpage_size), 0, sizeof(int));

	run_race(q_sync, race_type);
	if (test_flag == 0)
		tst_res(TPASS, "Test done");
}

static void setup(void)
{
	totpages = SAFE_READ_MEMINFO(MEMINFO_HPAGE_FREE);
	hpage_size = tst_get_hugepage_size();

	if (str_op)
		if (strcmp(str_op, "shared") == 0)
			race_type = MAP_SHARED;
		else if (strcmp(str_op, "private") == 0)
			race_type = MAP_PRIVATE;
		else
			tst_res(TFAIL, "Usage:mmap<private|shared>");
	else
		tst_res(TFAIL, "Usage:mmap<private|shared>");

	fd_sync = tst_creat_unlinked(MNTPOINT, 0);
	/* Get a shared normal page for synchronization */
	q_sync = SAFE_MMAP(NULL, getpagesize(), PROT_READ|PROT_WRITE,
			MAP_SHARED|MAP_ANONYMOUS, -1, 0);
	tst_res(TINFO, "Mapping %ld/%ld pages.. ", totpages-1, totpages);
	p_sync = SAFE_MMAP(NULL, (totpages-1)*hpage_size, PROT_READ|PROT_WRITE,
			MAP_SHARED, fd_sync, 0);
}

static void cleanup(void)
{
	if (fd_sync >= 0)
		SAFE_CLOSE(fd_sync);
	if (child1)
		SAFE_KILL(child1, SIGKILL);
	if (child2)
		SAFE_KILL(child2, SIGKILL);
}


static struct tst_test test = {
	.options = (struct  tst_option[]){
		{"m:", &str_op, "Usage:mmap<private|shared>"},
		{}
	},
	.needs_root = 1,
	.mntpoint = MNTPOINT,
	.needs_hugetlbfs = 1,
	.needs_tmpdir = 1,
	.setup = setup,
	.cleanup = cleanup,
	.test_all = run_test,
	.hugepages = {2, TST_NEEDS},
	.forks_child = 1,
	.min_cpus = 2
};
