// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) International Business Machines Corp., 2008
 *               13/11/08  Gowrishankar M <gowrishankar.m@in.ibm.com>
 * Copyright (C) 2022 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Clone a process with CLONE_NEWPID flag and check if kill(-1, signal) fails
 * with ESRCH when there are no process in the container.
 */

#include "tst_test.h"
#include "lapi/namespaces_constants.h"

static int child_func(LTP_ATTRIBUTE_UNUSED void *arg)
{
	pid_t cpid, ppid;
	int ret;

	cpid = getpid();
	ppid = getppid();

	if (cpid != 1 || ppid != 0) {
		tst_res(TFAIL, "Got unexpected result of cpid=%d ppid=%d", cpid, ppid);
		return 1;
	}

	ret = kill(-1, SIGUSR1);

	if (ret != -1 || errno != ESRCH) {
		tst_res(TFAIL, "kill() didn't fail with ESRCH");
		return 0;
	}

	tst_res(TPASS, "Can't kill processes from child namespace");

	return 0;
}

static void run(void)
{
	int ret;

	ret = ltp_clone_quick(CLONE_NEWPID | SIGCHLD, child_func, 0);
	if (ret < 0)
		tst_brk(TBROK | TERRNO, "clone failed");
}

static struct tst_test test = {
	.test_all = run,
	.needs_root = 1,
};
