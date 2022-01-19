// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2017 FUJITSU LIMITED. All rights reserved.
 * Author: Guangwen Feng <fenggw-fnst@cn.fujitsu.com>
 */

/*
 * This is a regression test for a crash caused by memcg function
 * reentrant on buggy kernel.  When doing rmdir(), a pending signal can
 * interrupt the execution and lead to cgroup_clear_css_refs()
 * being entered repeatedly, this results in a BUG_ON().
 */

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/mount.h>
#include "tst_test.h"
#include "tst_cgroup.h"


static volatile int sigcounter;
static const struct tst_cgroup_group *cg;
static struct tst_cgroup_group *test_cg;

static void sighandler(int sig LTP_ATTRIBUTE_UNUSED)
{
	sigcounter++;
}

static void do_child(void)
{
	while (1)
		SAFE_KILL(getppid(), SIGUSR1);

	exit(0);
}

static void do_test(void)
{
	pid_t cpid;

	SAFE_SIGNAL(SIGUSR1, sighandler);

	cpid = SAFE_FORK();
	if (cpid == 0)
		do_child();

	while (sigcounter < 50000) {
		test_cg = tst_cgroup_group_mk(cg, "test");

		if (test_cg)
			test_cg = tst_cgroup_group_rm(test_cg);
	}

	SAFE_KILL(cpid, SIGKILL);
	SAFE_WAIT(NULL);

	tst_res(TPASS, "Bug not reproduced");
}

static void setup(void)
{
	tst_cgroup_require("memory", NULL);

	cg = tst_cgroup_get_test_group();

	if (TST_CGROUP_VER(cg, "memory") != TST_CGROUP_V1)
                SAFE_CGROUP_PRINT(cg, "cgroup.subtree_control", "+memory");
}

static void cleanup(void)
{
	if (test_cg)
		test_cg = tst_cgroup_group_rm(test_cg);

	tst_cgroup_cleanup();
}

static struct tst_test test = {
	.needs_root = 1,
	.forks_child = 1,
	.min_kver = "2.6.24",
	.setup = setup,
	.cleanup = cleanup,
	.test_all = do_test,
};
