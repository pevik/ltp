// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines Corp., 2007
 *               13/11/08  Gowrishankar M  <gowrishankar.m@in.ibm.com>
 * Copyright (C) 2022 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Clone a process with CLONE_NEWPID flag, block SIGUSR1 signal before sending
 * it from parent and check if it's received once SIGUSR1 signal is unblocked.
 */

#define _GNU_SOURCE 1
#include <signal.h>
#include "tst_test.h"
#include "lapi/namespaces_constants.h"

static int signals;
static int last_signo;

static void child_signal_handler(LTP_ATTRIBUTE_UNUSED int sig, siginfo_t *si, LTP_ATTRIBUTE_UNUSED void *unused)
{
	last_signo = si->si_signo;
	signals++;
}

static int child_func(LTP_ATTRIBUTE_UNUSED void *arg)
{
	struct sigaction sa;
	sigset_t newset;
	pid_t cpid, ppid;

	cpid = getpid();
	ppid = getppid();

	if (cpid != 1 || ppid != 0) {
		tst_res(TFAIL, "Got unexpected result of cpid=%d ppid=%d", cpid, ppid);
		return 0;
	}

	SAFE_SIGEMPTYSET(&newset);
	SAFE_SIGADDSET(&newset, SIGUSR1);
	SAFE_SIGPROCMASK(SIG_BLOCK, &newset, 0);

	TST_CHECKPOINT_WAKE_AND_WAIT(0);

	sa.sa_flags = SA_SIGINFO;
	SAFE_SIGFILLSET(&sa.sa_mask);
	sa.sa_sigaction = child_signal_handler;

	SAFE_SIGACTION(SIGUSR1, &sa, NULL);

	SAFE_SIGPROCMASK(SIG_UNBLOCK, &newset, 0);

	if (signals != 1) {
		tst_res(TFAIL, "Received %d signals", signals);
		return 0;
	}

	if (last_signo != SIGUSR1) {
		tst_res(TFAIL, "Received %s signal", tst_strsig(last_signo));
		return 0;
	}

	tst_res(TPASS, "Received SIGUSR1 signal after unblock");

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
