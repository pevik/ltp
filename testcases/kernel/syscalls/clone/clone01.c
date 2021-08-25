// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Wipro Technologies Ltd, 2002.  All Rights Reserved.
 * Copyright (c) 2012 Wanlong Gao <gaowanlong@cn.fujitsu.com>
 */

/*\
 * [Description]
 * This is a test for the clone(2) system call.
 * It is intended to provide a limited exposure of the system call.
 */

#include <stdlib.h>
#include "tst_test.h"
#include "clone_platform.h"

static void *child_stack;

static int do_child(void *arg LTP_ATTRIBUTE_UNUSED)
{
	exit(0);
}

static void verify_clone(void)
{
	int status, child_pid;

	TST_EXP_POSITIVE(ltp_clone(SIGCHLD, do_child, NULL,
		CHILD_STACK_SIZE, child_stack), "clone()");

	child_pid = SAFE_WAIT(&status);

	if (TST_RET == child_pid)
		tst_res(TPASS, "clone returned %ld", TST_RET);
	else
		tst_res(TFAIL, "clone returned %ld, wait returned %d",
			 TST_RET, child_pid);
}

static void setup(void) {
	child_stack = SAFE_MALLOC(CHILD_STACK_SIZE);
}

static void cleanup(void) {
	free(child_stack);
}

static struct tst_test test = {
	.setup = setup,
	.cleanup = cleanup,
	.test_all = verify_clone,
	.forks_child = 1,
//	.needs_tmpdir = 1,
};