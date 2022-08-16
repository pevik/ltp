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

static volatile int exp_pid;
static volatile int passed;

static void child_signal_handler(LTP_ATTRIBUTE_UNUSED int sig, siginfo_t *si, LTP_ATTRIBUTE_UNUSED void *unused)
{
	tst_res(TINFO, "Received signal %s from PID %d", tst_strsig(si->si_signo), si->si_pid);

	if (si->si_signo != SIGUSR1)
		return;

	if (si->si_pid == exp_pid)
		passed = 1;
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

	passed = 0;
	exp_pid = 0;

	TST_CHECKPOINT_WAKE_AND_WAIT(0);

	if (passed)
		tst_res(TPASS, "Container resumed after receiving SIGUSR1 from parent namespace");
	else
		tst_res(TFAIL, "Container did not receive the SIGUSR1 signal from parent");

	passed = 0;
	exp_pid = 1;

	SAFE_KILL(cpid, SIGUSR1);

	if (passed)
		tst_res(TPASS, "Container resumed after receiving SIGUSR1 from container namespace");
	else
		tst_res(TFAIL, "Container did not receive the SIGUSR1 signal from container");

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
