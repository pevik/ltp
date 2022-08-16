// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) International Business Machines Corp., 2007
 *		08/10/08 Veerendra C <vechandr@in.ibm.com>
 * Copyright (C) 2022 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Clone a process with CLONE_NEWPID flag and verify that is not possible to
 * kill init process by sending SIGKILL from child namespace.
 */

#include <sys/wait.h>
#include "tst_test.h"
#include "lapi/namespaces_constants.h"

static int child_func(LTP_ATTRIBUTE_UNUSED void *arg)
{
	pid_t cpid, ppid;

	cpid = getpid();
	ppid = getppid();

	if (cpid != 1 || ppid != 0) {
		tst_res(TFAIL, "got unexpected result of cpid=%d ppid=%d", cpid, ppid);
		return 1;
	}

	tst_res(TINFO, "Sending SIGKILL to container init process from child");

	SAFE_KILL(1, SIGKILL);

	tst_res(TPASS, "Child namespace is still alive");

	return 0;
}

static void run(void)
{
	int ret, status;

	ret = ltp_clone_quick(CLONE_NEWPID | SIGCHLD, child_func, NULL);
	if (ret < 0)
		tst_brk(TBROK | TERRNO, "clone failed");

	SAFE_WAITPID(ret, &status, 0);

	if (WIFSIGNALED(status) && WTERMSIG(status) == SIGKILL) {
		tst_res(TFAIL, "Child killed by SIGKILL");
		return;
	}
}

static struct tst_test test = {
	.test_all = run,
	.needs_root = 1,
};
