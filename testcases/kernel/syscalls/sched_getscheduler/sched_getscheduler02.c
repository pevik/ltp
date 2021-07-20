// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2021, BELLSOFT. All rights reserved.
 * Copyright (c) International Business Machines  Corp., 2001
 */

/*\
 * [Description]
 *
 * Pass an invalid pid to sched_getscheduler() and test for ESRCH.
 */

#include <stdio.h>
#include <errno.h>

#include "tst_test.h"
#include "tst_sched.h"

static pid_t unused_pid;

static void setup(void)
{
	unused_pid = tst_get_unused_pid();
}

static void run(void)
{
	TST_EXP_FAIL(tst_sched_getscheduler(unused_pid), ESRCH);
}

static struct tst_test test = {
	.setup = setup,
	.test_all = run,
};
