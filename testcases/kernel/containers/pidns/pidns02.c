// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines Corp., 2007
 * Copyright (C) 2022 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Clone a process with CLONE_NEWPID flag and check:
 *
 * - child session ID must be 1
 * - parent process group ID must be 1
 */

#include "tst_test.h"
#include "lapi/sched.h"

static void child_func(void)
{
	pid_t sid, pgid;

	SAFE_SETSID();

	sid = getsid(0);
	pgid = getpgid(0);

	TST_EXP_EQ_LI(sid, 1);
	TST_EXP_EQ_LI(pgid, 1);
}

static void run(void)
{
	const struct tst_clone_args args = { CLONE_NEWPID, SIGCHLD };

	if (!SAFE_CLONE(&args)) {
		child_func();
		return;
	}
}

static struct tst_test test = {
	.test_all = run,
	.needs_root = 1,
	.forks_child = 1,
	.needs_kconfigs = (const char *[]) {
		"CONFIG_PID_NS",
		NULL,
	},
};
