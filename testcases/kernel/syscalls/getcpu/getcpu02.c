// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2024 FUJITSU LIMITED. All Rights Reserved.
 * Copyright (c) Linux Test Project, 2024
 * Author: Ma Xinjian <maxj.fnst@fujitsu.com>
 *
 */

/*\
 * [Description]
 *
 * Verify that getcpu(2) fails with
 *
 * - EFAULT arguments point outside the calling process's address
 *          space.
 */

#define _GNU_SOURCE
#include "tst_test.h"
#include "lapi/sched.h"

static void run(void)
{
	unsigned int cpu_id, node_id = 0;

	TST_EXP_FAIL(getcpu(tst_get_bad_addr(NULL), &node_id), EFAULT);
	TST_EXP_FAIL(getcpu(&cpu_id, tst_get_bad_addr(NULL)), EFAULT);
}

static struct tst_test test = {
	.test_all = run,
};
