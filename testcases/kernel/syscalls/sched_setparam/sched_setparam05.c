// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2021, BELLSOFT. All rights reserved.
 * Copyright (c) Wipro Technologies Ltd, 2002.  All Rights Reserved.
 * AUTHOR: Saji Kumar.V.R <saji.kumar@wipro.com>
 */

/*\
 * [Description]
 *
 * Verify that sched_setparam() fails if the user does not have proper
 * privileges.
 */

#include <stdlib.h>
#include <pwd.h>

#include "tst_test.h"
#include "tst_sched.h"

static void run(void)
{
	pid_t child_pid = SAFE_FORK();

	if (!child_pid) {
		struct sched_param p = { .sched_priority = 0 };
		struct passwd *pw = SAFE_GETPWNAM("nobody");

		SAFE_SETEUID(pw->pw_uid);
		TST_EXP_FAIL(tst_sched_setparam(getppid(), &p), EPERM);

		exit(0);
	}

	tst_reap_children();
}

static struct tst_test test = {
	.needs_root = 1,
	.forks_child = 1,
	.test_all = run,
};
