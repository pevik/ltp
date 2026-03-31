// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2015 Red Hat Inc.
 * Copyright (c) 2026 lufei <lufei@uniontech.com>
 */

/*\
 * Regression test for panic while using userstacktrace.
 *
 * BUG: unable to handle kernel paging request at 00000000417683c0
 *      IP: [<ffffffff8105c834>] update_curr+0x124/0x1e0
 *      Thread overran stack, or stack corrupted
 *      Oops: 0000 [#1] SMP
 *      last sysfs file: ../system/cpu/cpu15/cache/index2/shared_cpu_map
 *
 * The bug was fixed by:
 *      1dbd195 (tracing: Fix preempt count leak)
 */

#include <unistd.h>
#include <stdio.h>
#include "ftrace_regression.h"

#define DEBUGFS_DIR "debugfs"
#define STACK_TRACER_PATH "/proc/sys/kernel/stack_tracer_enabled"
#define TRACE_OPTIONS DEBUGFS_DIR "/tracing/trace_options"
#define EXC_PAGE_FAULT DEBUGFS_DIR "/tracing/events/exceptions/page_fault_kernel/enable"
#define MM_PAGE_FAULT DEBUGFS_DIR "/tracing/events/kmem/mm_kernel_pagefault/enable"

#define LOOP 10

static const char *page_fault_path;
static int page_fault_origin;

static void setup(void)
{
	SAFE_MKDIR(DEBUGFS_DIR, 0755);
	SAFE_MOUNT(NULL, DEBUGFS_DIR, "debugfs", 0, NULL);

	if (access(EXC_PAGE_FAULT, F_OK) == 0)
		page_fault_path = EXC_PAGE_FAULT;
	else if (access(MM_PAGE_FAULT, F_OK) == 0)
		page_fault_path = MM_PAGE_FAULT;
	else
		tst_brk(TCONF, "Page fault event not available");

	SAFE_FILE_SCANF(page_fault_path, "%d", &page_fault_origin);
}

static void run(void)
{
	int i;

	for (i = 0; i < LOOP; i++) {
		SAFE_FILE_PRINTF(STACK_TRACER_PATH, "1");
		SAFE_FILE_PRINTF(TRACE_OPTIONS, "userstacktrace");

		if (!file_contains(TRACE_OPTIONS, "^userstacktrace"))
			tst_brk(TBROK, "Failed to set userstacktrace");

		SAFE_FILE_PRINTF(page_fault_path, "1");
	}

	SAFE_FILE_PRINTF(page_fault_path, "%d", page_fault_origin);

	tst_res(TPASS, "Finished running the test");
}

static void cleanup(void)
{
	SAFE_UMOUNT(DEBUGFS_DIR);
}

static struct tst_test test = {
	.needs_root = 1,
	.needs_tmpdir = 1,
	.setup = setup,
	.test_all = run,
	.cleanup = cleanup,
	.save_restore = (const struct tst_path_val[]) {
		{STACK_TRACER_PATH, NULL, TST_SR_TCONF},
		{}
	},
	.tags = (const struct tst_tag[]) {
		{"linux-git", "1dbd195"},
		{}
	},
};
