// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Drop root privileges, create a container with CLONE_NEWUTS and verify that
 * we receive a permission error.
 */

#define _GNU_SOURCE

#include "tst_test.h"
#include "utsname.h"

static char *str_op = "clone";
static int use_clone;

static int child1_run(LTP_ATTRIBUTE_UNUSED void *vtest)
{
	return 0;
}

static void run(void)
{
	void *stack;
	size_t stack_size = getpagesize() * 6;

	stack = ltp_alloc_stack(stack_size);
	if (stack == NULL)
		tst_brk(TBROK, "Can't allocate stack");

	tst_res(TINFO, "Dropping root privileges");

	SAFE_SETRESUID(1000, 1000, 1000);

	tst_res(TINFO, "clone() with CLONE_NEWUTS");

	ltp_clone(CLONE_NEWUTS, child1_run, NULL, stack_size, stack);

	TST_EXP_PASS(errno == EPERM);
}

static void setup(void)
{
	use_clone = get_clone_unshare_enum(str_op);

	if (use_clone != T_CLONE && use_clone != T_UNSHARE)
		tst_brk(TCONF, "Only clone and unshare clone are supported");

	check_newuts();
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.needs_root = 1,
	.forks_child = 1,
	.needs_checkpoints = 1,
	.options = (struct tst_option[]) {
		{ "m:", &str_op, "Test execution mode <clone|unshare>" },
		{},
	},
};
