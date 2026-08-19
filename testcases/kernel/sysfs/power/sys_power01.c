// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2026 Cyril Hrubis <chrubis@suse.cz>
 */

/*\
 * Sanity checks for the power management attributes exported under
 * /sys/power/.
 *
 * The test verifies that:
 *
 * - pm_async, pm_debug_messages, pm_print_times and sync_on_suspend are
 *   booleans
 * - pm_freeze_timeout and wakeup_count are non-negative
 * - disk is a bracketed-choice file selecting one of platform, shutdown,
 *   reboot, suspend, test_resume or firmware
 * - mem_sleep is a bracketed-choice file selecting one of s2idle, shallow or
 *   deep
 * - state lists only known sleep states (freeze, standby, mem, disk, on)
 *
 * Unlike disk and mem_sleep, state never marks a "current" value with
 * brackets, it merely enumerates the sleep states the kernel supports, so it
 * is validated as a plain token set rather than a bracketed choice.
 *
 * All checks skip gracefully with TCONF when a particular attribute is not
 * present.
 */

#include <limits.h>
#include "tst_test.h"
#include "tst_sysfs_assert.h"

#define POWER "/sys/power"

static const char *const disk_allowed[] = {
	"platform", "shutdown", "reboot", "suspend", "test_resume",
	"firmware", NULL
};

static const char *const mem_sleep_allowed[] = {
	"s2idle", "shallow", "deep", NULL
};

static const char *const state_allowed[] = {
	"freeze", "standby", "mem", "disk", "on", NULL
};

static void do_test(void)
{
	TST_SYSFS_ASSERT_BOOL(POWER "/pm_async");
	TST_SYSFS_ASSERT_BOOL(POWER "/pm_debug_messages");
	TST_SYSFS_ASSERT_BOOL(POWER "/pm_print_times");
	TST_SYSFS_ASSERT_BOOL(POWER "/sync_on_suspend");

	TST_SYSFS_ASSERT_RANGELL(0, LONG_MAX, POWER "/pm_freeze_timeout");
	TST_SYSFS_ASSERT_RANGELL(0, LONG_MAX, POWER "/wakeup_count");

	TST_SYSFS_ASSERT_CHOICE(disk_allowed, NULL, 0, POWER "/disk");
	TST_SYSFS_ASSERT_CHOICE(mem_sleep_allowed, NULL, 0, POWER "/mem_sleep");

	TST_SYSFS_ASSERT_TOKENS(state_allowed, NULL, POWER "/state");
}

static struct tst_test test = {
	.test_all = do_test,
};
