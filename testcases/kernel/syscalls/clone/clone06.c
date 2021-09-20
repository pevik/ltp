// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Wipro Technologies Ltd, 2002.  All Rights Reserved.
 * Copyright (c) 2012 Wanlong Gao <gaowanlong@cn.fujitsu.com>
 */

/*\
 * [Description]
 * Test to verify inheritance of environment variables by child.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include "tst_test.h"
#include "clone_platform.h"

#define MAX_LINE_LENGTH 256

static void setup(void);
static void cleanup(void);
static int child_environ();

static int pfd[2];
static void *child_stack;

static int child_environ(void *arg LTP_ATTRIBUTE_UNUSED)
{
	char var[MAX_LINE_LENGTH];

	sprintf(var, "%s", getenv("TERM") ? : "");

	SAFE_CLOSE(pfd[0]);
	SAFE_WRITE(1, pfd[1], var, sizeof(var));
	SAFE_CLOSE(pfd[1]);

	exit(0);
}

static void verify_clone(void)
{
	char *parent_env;
	char buff[MAX_LINE_LENGTH];

	TST_EXP_POSITIVE(ltp_clone(SIGCHLD, child_environ, NULL, CHILD_STACK_SIZE,
				child_stack));

	tst_reap_children();

	if (!TST_PASS)
		return;

	SAFE_CLOSE(pfd[1]);
	SAFE_READ(1, pfd[0], buff, sizeof(buff));
	SAFE_CLOSE(pfd[0]);

	parent_env = getenv("TERM") ? : "";
	TST_EXP_PASS(strcmp(buff, parent_env),
				"verify environment variables by child");
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