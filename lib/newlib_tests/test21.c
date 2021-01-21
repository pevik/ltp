// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2020 Red Hat, Inc.
 * Copyright (c) 2020 Li Wang <liwang@redhat.com>
 */

/*
 * Tests tst_cgroup.h APIs
 */

#include "tst_test.h"
#include "tst_cgroup.h"

#define MEMSIZE 1024 * 1024

static void do_test(void)
{
	pid_t pid = SAFE_FORK();

	switch (pid) {
	case 0:
		tst_cgroup_move_current(TST_CGROUP_MEMORY);
		tst_cgroup_mem_set_maxbytes(MEMSIZE);
		tst_cgroup_mem_set_maxswap(MEMSIZE);
	break;
	default:
		tst_cgroup_move_current(TST_CGROUP_CPUSET);

		tst_cgroup_move_current(TST_CGROUP_MEMORY);
		tst_cgroup_mem_set_maxbytes(MEMSIZE);
		tst_cgroup_mem_set_maxswap(MEMSIZE);
	break;
	}

	tst_res(TPASS, "Cgroup mount test");
}

static void setup(void)
{
	tst_cgroup_require(TST_CGROUP_MEMORY, NULL);
}

static void cleanup(void)
{
}

static struct tst_test test = {
	.test_all = do_test,
	.setup = setup,
	.cleanup = cleanup,
	.forks_child = 1,
};
