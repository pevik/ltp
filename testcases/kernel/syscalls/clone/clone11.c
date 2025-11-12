// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 Stephen Bertram <sbertram@redhat.com>
 */

/*\
 * This test verifies that clone() fals with EPERM when CAP_SYS_ADMIN
 * has been dropped.
 */

#define _GNU_SOURCE

#include "tst_test.h"
#include "clone_platform.h"

static void *child_stack;
static int *child_pid;

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

static int child_fn(void *arg LTP_ATTRIBUTE_UNUSED)
{
		*child_pid = getpid();
		exit(0);
}

static void run(unsigned int n)
{
	struct tcase *tc = &tcases[n];

	TST_EXP_FAIL(ltp_clone(tc->flags, child_fn, NULL, CHILD_STACK_SIZE,
			child_stack), EPERM, "clone(%s) should fail with EPERM"
			, tc->sflags);
}

static void setup(void)
{
	child_pid = SAFE_MMAP(NULL, sizeof(*child_pid), PROT_READ | PROT_WRITE,
		MAP_SHARED | MAP_ANONYMOUS, -1, 0);
}

static void cleanup(void)
{
	if (child_pid)
		SAFE_MUNMAP(child_pid, sizeof(*child_pid));
}

static struct tst_test test = {
	.tcnt = ARRAY_SIZE(tcases),
	.setup = setup,
	.test = run,
	.cleanup = cleanup,
	.needs_root = 1,
	.caps = (struct tst_cap []) {
			TST_CAP(TST_CAP_DROP, CAP_SYS_ADMIN),
			{},
	},
	.bufs = (struct tst_buffers []) {
			{&child_stack, .size = CHILD_STACK_SIZE},
			{},
	},
};
