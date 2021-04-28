// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) Linux Test Project, 2009-2021
 * Copyright (c) Crackerjack Project, 2007
 * Ported from Crackerjack to LTP by Manas Kumar Nayak maknayak@in.ibm.com>
 */

/*\
 * [Description]
 *
 * Basic tests for the tkill syscall.
 *
 * [Algorithm]
 *
 * Calls tkill and capture signals to verify success.
 */

#include <signal.h>

#include "lapi/syscalls.h"
#include "tst_test.h"

static int sig_flag;

static void sighandler(int sig)
{
	if (sig == SIGUSR1)
		sig_flag = 1;
}

static void run(void)
{
	int tid;

	SAFE_SIGNAL(SIGUSR1, sighandler);

	tid = tst_syscall(__NR_gettid);

	TST_EXP_PASS(tst_syscall(__NR_tkill, tid, SIGUSR1));

	while (!sig_flag)
		usleep(1000);
}

static struct tst_test test = {
	.needs_tmpdir = 1,
	.test_all = run,
};
