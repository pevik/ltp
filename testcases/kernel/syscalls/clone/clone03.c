// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Wipro Technologies Ltd, 2002.  All Rights Reserved.
 * Copyright (c) 2012 Wanlong Gao <gaowanlong@cn.fujitsu.com>
 */

/*\
 * [Description]
 *	Check for equality of pid of child & return value of clone(2)
 */
#include <stdio.h>
#include <stdlib.h>
#include "tst_test.h"
#include "clone_platform.h"

static int pfd[2];
static void *child_stack;

static int child_fn(void *arg LTP_ATTRIBUTE_UNUSED)
{
	char pid[10];

	sprintf(pid, "%d", getpid());

	SAFE_CLOSE(pfd[0]);
	SAFE_WRITE(1, pfd[1], pid, sizeof(pid));
	SAFE_CLOSE(pfd[1]);

	exit(0);
}

static void verify_clone(void)
{
	char buff[10];
	int child_pid;

	TST_EXP_POSITIVE(ltp_clone(SIGCHLD, child_fn, NULL, CHILD_STACK_SIZE,
				child_stack));

	if (!TST_PASS)
		return;

	tst_reap_children();

	SAFE_CLOSE(pfd[1]);
	SAFE_READ(1, pfd[0], buff, sizeof(buff));
	SAFE_CLOSE(pfd[0]);
	child_pid = atoi(buff);

	TST_EXP_PASS(!(TST_RET == child_pid), "pid(%d)", child_pid);
}

static void setup(void)
{
	child_stack = SAFE_MALLOC(CHILD_STACK_SIZE);
	SAFE_PIPE(pfd);
}

static void cleanup(void)
{
	free(child_stack);
}

static struct tst_test test = {
	.setup = setup,
	.test_all = verify_clone,
	.cleanup = cleanup,
};
