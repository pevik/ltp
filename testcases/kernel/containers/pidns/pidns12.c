// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines Corp., 2007
 *               13/11/08  Gowrishankar M <gowrishankar.m@in.ibm.com>
 * Copyright (C) 2022 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Clone a process with CLONE_NEWPID flag and verifies that siginfo->si_pid is
 * set to 0 if sender (parent process) is not in the receiver's namespace.
 */

#define _GNU_SOURCE 1
#include <signal.h>
#include "tst_test.h"
#include "lapi/namespaces_constants.h"

static void child_signal_handler(LTP_ATTRIBUTE_UNUSED int sig, siginfo_t *si, LTP_ATTRIBUTE_UNUSED void *unused)
{
	if (si->si_pid == 0)
		tst_res(TPASS, "signalling PID (from other namespace) is 0 as expected");
	else
		tst_res(TFAIL, "signalling PID (from other namespace) is not 0, but %d.", si->si_pid);
}

static int child_func(LTP_ATTRIBUTE_UNUSED void *arg)
{
	struct sigaction sa;
	pid_t cpid, ppid;

	cpid = getpid();
	ppid = getppid();

	if (cpid != 1 || ppid != 0) {
		tst_res(TFAIL, "Got unexpected result of cpid=%d ppid=%d", cpid, ppid);
		return 1;
	}

	sa.sa_flags = SA_SIGINFO;
	SAFE_SIGFILLSET(&sa.sa_mask);
	sa.sa_sigaction = child_signal_handler;

	SAFE_SIGACTION(SIGUSR1, &sa, NULL);

	TST_CHECKPOINT_WAKE_AND_WAIT(0);

	return 0;
}

static void run(void)
{
	int ret;

	ret = ltp_clone_quick(CLONE_NEWPID | SIGCHLD, child_func, NULL);
	if (ret < 0)
		tst_brk(TBROK | TERRNO, "clone failed");

	TST_CHECKPOINT_WAIT(0);

	SAFE_KILL(ret, SIGUSR1);

	TST_CHECKPOINT_WAKE(0);
}

static struct tst_test test = {
	.test_all = run,
	.needs_root = 1,
	.needs_checkpoints = 1,
};
