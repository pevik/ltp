// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines  Corp., 2001
 * Copyright (C) 2026 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 *  Fork a process using vfork() and verify that, the pending signals in
 *  the parent are not pending in the child process.
 */

#include "tst_test.h"

static void run(void)
{
	if (!vfork()) {
		sigset_t signal;

		if (sigpending(&signal) == -1)
			tst_brk(TBROK | TERRNO, "sigpending() error");

		TST_EXP_EQ_LI(sigismember(&signal, SIGUSR1), 0);

		exit(0);
	}
}

static void sig_handler(LTP_ATTRIBUTE_UNUSED int signo)
{
}

static void setup(void)
{
	struct sigaction action;

	memset(&action, 0, sizeof(action));
	action.sa_handler = sig_handler;

	SAFE_SIGACTION(SIGUSR1, &action, NULL);
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
};
