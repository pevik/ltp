// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 Stephen Bertram <sbertram@redhat.com>
 */

/*\
 * This test verifies that clone3() fals with EPERM when CAP_SYS_ADMIN
 * has been dropped and ``clone_args.set_tid_size`` is greater than zero.
 */

#define _GNU_SOURCE
#include "tst_test.h"
#include "lapi/sched.h"

static struct clone_args args = {0};
static pid_t tid_array[4] = {0, 0, 0, 0};

static struct tcase {
	uint64_t flags;
	char *sflags;
} tcases[] = {
	{CLONE_NEWPID, "CLONE_NEWPID"},
	{CLONE_NEWCGROUP, "CLONE_NEWCGROUP"},
	{CLONE_NEWIPC, "CLONE_NEWIPC"},
	{CLONE_NEWNET, "CLONE_NEWNET"},
	{CLONE_NEWNS, "CLONE_NEWNS"},
	{CLONE_NEWUTS, "CLONE_NEWUTS"},
};

static void run(unsigned int n)
{
	struct tcase *tc = &tcases[n];

	args.flags = tc->flags;

	TST_EXP_FAIL(clone3(&args, sizeof(args)), EPERM, "clone3(%s) should fail with EPERM"
			, tc->sflags);
}

static void setup(void)
{
	clone3_supported_by_kernel();

	memset(&args, 0, sizeof(args));
	args.set_tid = (uintptr_t)tid_array;
	/* set_tid_size greater than zero - requires CAP_SYS_ADMIN */
	args.set_tid_size = 4;
}

static struct tst_test test = {
	.tcnt = ARRAY_SIZE(tcases),
	.setup = setup,
	.test = run,
	.needs_root = 1,
	.caps = (struct tst_cap []) {
			TST_CAP(TST_CAP_DROP, CAP_SYS_ADMIN),
			{},
	},
	.bufs = (struct tst_buffers []) {
			{&args, .size = sizeof(struct clone_args)},
			{},
	},
};
