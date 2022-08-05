// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) International Business Machines Corp., 2007
 *		08/10/08 Veerendra C <vechandr@in.ibm.com>
 * Copyright (C) 2022 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Clone a process with CLONE_NEWPID flag and create many levels of child
 * containers. Then kill container init process from parent and check if all
 * containers have been killed.
 */

#include <sys/wait.h>
#include "tst_test.h"
#include "lapi/namespaces_constants.h"

#define MAX_DEPTH	5

static pid_t pid_max;

static int child_func(void *arg)
{
	int ret;
	int *level;
	pid_t cpid, ppid;

	cpid = getpid();
	ppid = getppid();

	if (cpid != 1 || ppid != 0) {
		tst_res(TFAIL, "Got unexpected result of cpid=%d ppid=%d", cpid, ppid);
		return 1;
	}

	level = (int *)arg;

	if (*level >= MAX_DEPTH) {
		TST_CHECKPOINT_WAKE(0);
		return 0;
	}

	(*level)++;

	ret = ltp_clone_quick(CLONE_NEWPID | SIGCHLD, child_func, level);
	if (ret < 0)
		tst_brk(TBROK | TERRNO, "clone failed");

	pause();

	return 0;
}

static int find_cinit_pids(pid_t *pids)
{
	int i;
	int next = 0;
	pid_t parentpid, pgid, pgid2;

	parentpid = getpid();
	pgid = getpgid(parentpid);

	for (i = parentpid + 1; i < pid_max; i++) {
		pgid2 = getpgid(i);

		if (pgid2 == pgid) {
			pids[next] = i;
			next++;
		}
	}

	return next;
}

static void setup(void)
{
	SAFE_FILE_SCANF("/proc/sys/kernel/pid_max", "%d\n", &pid_max);
}

static void run(void)
{
	int ret, i;
	int status;
	int children;
	int level = 0;
	pid_t pids[MAX_DEPTH];
	pid_t pids_new[MAX_DEPTH];

	ret = ltp_clone_quick(CLONE_NEWPID | SIGCHLD, child_func, &level);
	if (ret < 0)
		tst_brk(TBROK | TERRNO, "clone failed");

	TST_CHECKPOINT_WAIT(0);

	find_cinit_pids(pids);

	SAFE_KILL(pids[0], SIGKILL);

	TST_RETRY_FUNC(waitpid(0, &status, WNOHANG), TST_RETVAL_NOTNULL);

	children = find_cinit_pids(pids_new);

	if (children > 0) {
		tst_res(TFAIL, "%d children left after sending SIGKILL", children);

		for (i = 0; i < MAX_DEPTH; i++) {
			kill(pids[i], SIGKILL);
			waitpid(pids[i], &status, 0);
		}

		return;
	}

	tst_res(TPASS, "No children left after sending SIGKILL to the first child");
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.needs_root = 1,
	.needs_checkpoints = 1,
};
