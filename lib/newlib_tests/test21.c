// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2020 Red Hat, Inc.
 * Copyright (c) 2020 Li Wang <liwang@redhat.com>
 */

/*
 * Tests tst_cgroup.h APIs
 */

#include <stdlib.h>

#include "tst_test.h"
#include "tst_cgroup.h"

#define MEMSIZE 1024 * 1024

static const struct tst_cgroup *cg;

static void do_test(void)
{
	pid_t pid = SAFE_FORK();

	switch (pid) {
	case 0:
		SAFE_CGROUP_PRINTF(&cg->cgroup.procs, "%d", getpid());
		SAFE_CGROUP_PRINTF(&cg->memory.max, "%d", MEMSIZE);
		if (TST_CGROUP_HAS(&cg->memory.swap))
			SAFE_CGROUP_PRINTF(&cg->memory.swap.max, "%d", MEMSIZE);
		exit(0);
		break;
	default:
		SAFE_CGROUP_PRINTF(&cg->cgroup.procs, "%d", getpid());
	break;
	}

	tst_reap_children();
	tst_res(TPASS, "Cgroup test");
}

static void setup(void)
{
	/* Omitting the below causes a warning */
	/* tst_cgroup_require(TST_CGROUP_CPUSET, NULL); */
	tst_cgroup_require(TST_CGROUP_MEMORY, NULL);
	cg = tst_cgroup_get_default();
}

static void cleanup(void)
{
	tst_cgroup_cleanup();
}

static struct tst_test test = {
	.test_all = do_test,
	.setup = setup,
	.cleanup = cleanup,
	.forks_child = 1,
};
