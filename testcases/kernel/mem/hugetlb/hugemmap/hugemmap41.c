// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2005-2006 IBM Corporation.
 * Author: David Gibson & Adam Litke
 */
/*\
 * [Description]
 *
 * ppc64 kernels (prior to 2.6.15-rc5) have a bug in the hugepage SLB
 * flushing path.  After opening new hugetlb areas, we update the
 * masks in the thread_struct, copy to the PACA, then do slbies on
 * each CPU.  The trouble is we only copy to the PACA on the CPU where
 * we're opening the segments, which can leave a stale copy in the
 * PACAs on other CPUs.
 *
 * This can be triggered either with multiple threads sharing the mm,
 * or with a single thread which is migrated from one CPU, to another
 * (where the mapping occurs), then back again (where we touch the
 * stale SLB).  We use the second method in this test, since it's
 * easier to force (using sched_setaffinity).  However it relies on a
 * close-to-idle system, if any process other than a kernel thread
 * runs on the first CPU between runs of the test process, the SLB
 * will be flushed and we won't trigger the bug, hence the
 * PASS_INCONCLUSIVE().  Obviously, this test won't work on a 1-cpu
 * system (should get CONFIG() on the sched_setaffinity)
 *
 */
#define _GNU_SOURCE
#include "hugetlb.h"
#define SYSFS_CPU_ONLINE_FMT	"/sys/devices/system/cpu/cpu%d/online"
#define MNTPOINT "hugetlbfs/"


#include <stdio.h>
#include <sched.h>


long hpage_size;
int fd;
void *p;
volatile unsigned long *q;
int online_cpus[2], err;
cpu_set_t cpu0, cpu1;



void check_online_cpus(int online_cpus[], int nr_cpus_needed)
{
	char cpu_state, path_buf[64];
	int total_cpus, cpu_idx, fd, ret, i;

	total_cpus = get_nprocs_conf();
	cpu_idx = 0;

	if (get_nprocs() < nr_cpus_needed)
		tst_res(TFAIL, "Atleast online %d cpus are required", nr_cpus_needed);

	for (i = 0; i < total_cpus && cpu_idx < nr_cpus_needed; i++) {
		errno = 0;
		sprintf(path_buf, SYSFS_CPU_ONLINE_FMT, i);
		fd = open(path_buf, O_RDONLY);
		if (fd < 0) {
			/* If 'online' is absent, the cpu cannot be offlined */
			if (errno == ENOENT) {
				online_cpus[cpu_idx] = i;
				cpu_idx++;
				continue;
			} else {
				tst_res(TFAIL, "Unable to open %s: %s", path_buf,
				     strerror(errno));
			}
		}

		ret = read(fd, &cpu_state, 1);
		if (ret < 1)
			tst_res(TFAIL, "Unable to read %s: %s", path_buf,
			     strerror(errno));

		if (cpu_state == '1') {
			online_cpus[cpu_idx] = i;
			cpu_idx++;
		}

		if (fd >= 0)
			SAFE_CLOSE(fd);
	}

	if (cpu_idx < nr_cpus_needed)
		tst_res(TFAIL, "Atleast %d online cpus were not found", nr_cpus_needed);
}


static void run_test(void)
{
	check_online_cpus(online_cpus, 2);
	CPU_ZERO(&cpu0);
	CPU_SET(online_cpus[0], &cpu0);
	CPU_ZERO(&cpu1);
	CPU_SET(online_cpus[1], &cpu1);

	err = sched_setaffinity(getpid(), CPU_SETSIZE/8, &cpu0);
	if (err != 0)
		tst_res(TFAIL, "sched_setaffinity(cpu%d): %s", online_cpus[0],
				strerror(errno));

	err = sched_setaffinity(getpid(), CPU_SETSIZE/8, &cpu1);

	if (err != 0)
		tst_res(TFAIL, "sched_setaffinity(cpu%d): %s", online_cpus[1],
				strerror(errno));
	p = SAFE_MMAP(NULL, hpage_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

	err = sched_setaffinity(getpid(), CPU_SETSIZE/8, &cpu0);
	if (err != 0)
		tst_res(TFAIL, "sched_setaffinity(cpu%d): %s", online_cpus[0],
				strerror(errno));
	q = (volatile unsigned long *)(p + getpagesize());
	*q = 0xdeadbeef;

	tst_res(TPASS, "Test Passed inconclusive");
}

static void setup(void)
{
	hpage_size = tst_get_hugepage_size();
	fd = tst_creat_unlinked(MNTPOINT, 0);
}

void cleanup(void)
{
	if (fd > 0)
		SAFE_CLOSE(fd);
}

static struct tst_test test = {
	.needs_root = 1,
	.mntpoint = MNTPOINT,
	.needs_hugetlbfs = 1,
	.needs_tmpdir = 1,
	.setup = setup,
	.cleanup = cleanup,
	.test_all = run_test,
	.hugepages = {1, TST_NEEDS},
};
