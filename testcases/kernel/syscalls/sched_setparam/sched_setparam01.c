// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2021, BELLSOFT. All rights reserved.
 * Copyright (c) Wipro Technologies Ltd, 2002.  All Rights Reserved.
 * AUTHOR: Saji Kumar.V.R <saji.kumar@wipro.com>
 */

/*\
 * [Description]
 *
 * Basic test for sched_setparam(2)
 *
 * Call sched_setparam(2) with pid=0 so that it will
 * set the scheduling parameters for the calling process
 */

#include "tst_test.h"
#include "tst_sched.h"

static void run(void)
{
	struct sched_param p = { .sched_priority = 0 };

	TST_EXP_PASS(tst_sched_setparam(0, &p));
}

static struct tst_test test = {
	.test_all = run,
};
