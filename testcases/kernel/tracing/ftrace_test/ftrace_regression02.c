// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2015 Red Hat Inc.
 * Copyright (c) 2026 lufei <lufei@uniontech.com>
 */

/*\
 * Check signal:signal_generate gives 2 more fields: grp=[0-9] res=[0-9]
 */

#include <unistd.h>
#include <stdio.h>
#include "ftrace_regression.h"

#define DEBUGFS_DIR "debugfs"
#define SET_EVENT DEBUGFS_DIR "/tracing/set_event"
#define TRACING_ON DEBUGFS_DIR "/tracing/tracing_on"
#define TRACE_FILE DEBUGFS_DIR "/tracing/trace"

#define LOOP 100

static void setup(void)
{
	SAFE_MKDIR(DEBUGFS_DIR, 0755);
	SAFE_MOUNT(NULL, DEBUGFS_DIR, "debugfs", 0, NULL);
}

static void run(void)
{
	int i;

	SAFE_FILE_PRINTF(SET_EVENT, "signal:signal_generate");
	SAFE_FILE_PRINTF(TRACING_ON, "1");
	SAFE_FILE_PRINTF(TRACE_FILE, "\n");

	for (i = 0; i < LOOP; i++)
		tst_cmd((const char *[]){"ls", "-l", "/proc", NULL},
			"/dev/null", "/dev/null", 0);

	if (file_contains(TRACE_FILE, "grp=[0-9]") && file_contains(TRACE_FILE, "res=[0-9]"))
		tst_res(TPASS, "Finished running the test");
	else
		tst_res(TFAIL, "Pattern grp=[0-9] res=[0-9] not found in trace");
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
	.needs_cmds = (struct tst_cmd[]) {
		{.cmd = "ls"},
		{}
	},
	.min_kver = "3.2",
	.tags = (const struct tst_tag[]) {
		{"linux-git", "6c303d3"},
		{"linux-git", "163566f"},
		{}
	},
};
