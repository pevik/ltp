// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines Corp., 2007
 *               13/11/08  Gowrishankar M <gowrishankar.m@in.ibm.com>
 * Copyright (C) 2022 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Clone a process with CLONE_NEWPID flag and spawn many children inside the
 * container. Then terminate all children and check if they ended up.
 */

#include <sys/wait.h>
#include "tst_test.h"
#include "lapi/namespaces_constants.h"

#define CHILDREN_NUM 10

static int child_func(LTP_ATTRIBUTE_UNUSED void *arg)
{
	int children[CHILDREN_NUM], status;
	unsigned int i, counter = 0;
	pid_t cpid, ppid;

	cpid = getpid();
	ppid = getppid();

	if (cpid != 1 || ppid != 0) {
		tst_res(TFAIL, "Got unexpected result of cpid=%d ppid=%d", cpid, ppid);
		return 1;
	}

	for (i = 0; i < CHILDREN_NUM; i++) {
		children[i] = SAFE_FORK();
		if (!children[i]) {
			pause();
			return 0;
		}
	}

	SAFE_KILL(-1, SIGUSR1);

	for (i = 0; i < CHILDREN_NUM; i++) {
		SAFE_WAITPID(children[i], &status, 0);

		if (WIFSIGNALED(status) && WTERMSIG(status) == SIGUSR1)
			counter++;
	}

	if (counter != CHILDREN_NUM) {
		tst_res(TFAIL, "%d children didn't end", CHILDREN_NUM - counter);
		return 1;
	}

	tst_res(TPASS, "All children ended successfuly");

	return 0;
}

static void run(void)
{
	int ret;

	ret = ltp_clone_quick(CLONE_NEWPID | SIGCHLD, child_func, NULL);
	if (ret < 0)
		tst_brk(TBROK | TERRNO, "clone failed");
}

static struct tst_test test = {
	.test_all = run,
	.needs_root = 1,
	.forks_child = 1,
};
