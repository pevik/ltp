// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (c) 2009 FUJITSU LIMITED  Miao Xie <miaox@cn.fujitsu.com>
 * Copyright (c) 2023 SUSE LLC <wegao@suse.com>
 */

/*\
 * [Description]
 *
 * This test check cpuset's mems work with hugepage file.
 *
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <sys/mount.h>
#include <limits.h>
#include <sys/param.h>
#include <sys/types.h>
#include "tst_test.h"
#include "../../controllers/cpuset/cpuset_lib/cpuset.h"

#ifdef HAVE_NUMA_V2
#include <numa.h>
#include <numaif.h>
#include <errno.h>
#include "tst_numa.h"
#include "tst_safe_stdio.h"
#include "mem.h"
#include "numa_helper.h"

#define MNTPOINT "hugetlbfs/"
#define HUGE_PAGE_FILE MNTPOINT "hugepagefile"
#define PAGES_ALLOCATED 16u

static long hpage_size;
static struct tst_nodemap *node;
static struct tst_cg_group *cg_cpuset_0;

static void count_cpus_mems(void)
{
	node = tst_get_nodemap(TST_NUMA_MEM, PAGES_ALLOCATED * getpagesize() / 1024);
	if (node->cnt <= 1)
		tst_brk(TCONF, "test requires NUMA system");
}

static void touch_memory_and_check_node(char *p, int size)
{
	int i;
	int pagesize = sysconf(_SC_PAGESIZE);

	for (i = 0; i < size; i += pagesize)
		p[i] = 0xef;

	if (cpuset_addr2node(p) == 0)
		tst_res(TPASS, "check node pass");
	else
		tst_res(TFAIL, "check node failed");
}

static void child(void)
{
	char *p;
	int fd_hugepage;

	TST_CHECKPOINT_WAIT(0);

	fd_hugepage = SAFE_OPEN(HUGE_PAGE_FILE, O_CREAT | O_RDWR, 0755);
	p = SAFE_MMAP(NULL, hpage_size, PROT_WRITE | PROT_READ,
				MAP_SHARED, fd_hugepage, 0);

	touch_memory_and_check_node(p, hpage_size);

	SAFE_MUNMAP(p, hpage_size);
	SAFE_CLOSE(fd_hugepage);

	TST_CHECKPOINT_WAKE(1);
}

static void run_test(void)
{
	int pid;

	cg_cpuset_0 = tst_cg_group_mk(tst_cg, "0");

	SAFE_CG_PRINT(cg_cpuset_0, "cpuset.cpus", "0");
	SAFE_CG_PRINT(cg_cpuset_0, "cpuset.mems", "0");
	SAFE_CG_PRINT(cg_cpuset_0, "cpuset.sched_load_balance", "0");

	SAFE_FILE_PRINTF("/proc/sys/vm/nr_hugepages", "%d", 2 * node->cnt);

	pid = SAFE_FORK();
	if (!pid) {
		child();
	} else {
		SAFE_CG_PRINTF(cg_cpuset_0, "cgroup.procs", "%d", pid);

		TST_CHECKPOINT_WAKE(0);
		TST_CHECKPOINT_WAIT(1);

		SAFE_WAITPID(pid, NULL, 0);

		cg_cpuset_0 = tst_cg_group_rm(cg_cpuset_0);
	}
}

static void setup(void)
{
	count_cpus_mems();

	hpage_size = SAFE_READ_MEMINFO(MEMINFO_HPAGE_SIZE)*1024;
}

static void cleanup(void)
{
	if (cg_cpuset_0)
		cg_cpuset_0 = tst_cg_group_rm(cg_cpuset_0);
}

static struct tst_test test = {
	.needs_root = 1,
	.runs_script = 1,
	.mntpoint = MNTPOINT,
	.needs_hugetlbfs = 1,
	.setup = setup,
	.forks_child = 1,
	.cleanup = cleanup,
	.test_all = run_test,
	.hugepages = {3, TST_NEEDS},
	.needs_checkpoints = 1,
	.needs_cgroup_ctrls = (const char *const []){ "cpuset", NULL },
	.save_restore = (const struct tst_path_val[]) {
		{"/proc/sys/vm/nr_hugepages", NULL, TST_SR_TBROK},
		{}
	},
};

#else
TST_TEST_TCONF(NUMA_ERROR_MSG);
#endif
