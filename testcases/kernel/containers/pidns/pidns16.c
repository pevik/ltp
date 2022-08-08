// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines Corp., 2007
 *               04/11/08  Veerendra C  <vechandr@in.ibm.com>
 * Copyright (C) 2022 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Clone a process with CLONE_NEWPID flag and verifies that siginfo->si_pid is
 * set to 0 if sender (parent process) sent the signal. Then send signal from
 * container itself and check if siginfo->si_pid is set to 1.
 */

#define _GNU_SOURCE 1
#include <signal.h>
#include "tst_test.h"
#include "lapi/namespaces_constants.h"

static int counter;

static void child_signal_handler(LTP_ATTRIBUTE_UNUSED int sig, siginfo_t *si, LTP_ATTRIBUTE_UNUSED void *unused)
{
	pid_t exp_pid;

	switch (counter) {
	case 0:
		exp_pid = 0;
		break;
	case 1:
		exp_pid = 1;
		break;
	default:
		tst_brk(TBROK, "Child should NOT be signalled 3+ times");
		return;
	}

	if (si->si_pid == exp_pid)
		tst_res(TPASS, "Signalling PID is %d as expected", exp_pid);
	else
		tst_res(TFAIL, "Signalling PID is not %d, but %d", exp_pid, si->si_pid);

	counter++;
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

	tst_res(TINFO, "Container: Resumed after receiving SIGUSR1 from parent namespace");

	SAFE_KILL(cpid, SIGUSR1);

	tst_res(TINFO, "Container: Resumed after sending SIGUSR1 from container itself");

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
