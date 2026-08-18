// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2026 Cyril Hrubis <chrubis@suse.cz>
 */

/*\
 * Sanity checks for the SMT (simultaneous multithreading) control exported
 * under /sys/devices/system/cpu/smt/.
 *
 * The test verifies that:
 *
 * - active is a boolean (0 or 1)
 * - control is one of the known states (on, off, forceoff, notsupported,
 *   notimplemented)
 * - when SMT is not usable (control is notsupported or notimplemented) active
 *   reads back as 0
 *
 * The test skips with TCONF when the smt directory is not present.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "tst_test.h"
#include "tst_sysfs_assert.h"

#define SMT "/sys/devices/system/cpu/smt"

static const char *const control_allowed[] = {
	"on", "off", "forceoff", "notsupported", "notimplemented", NULL
};

static void do_test(void)
{
	char control[32];
	long active;

	if (access(SMT, F_OK)) {
		tst_res(TCONF, SMT " is not available");
		return;
	}

	TST_SYSFS_ASSERT_BOOL(SMT "/active");

	TST_SYSFS_ASSERT_ONEOF(control_allowed, SMT "/control");

	TST_SYSFS_READ_STR(control, sizeof(control), SMT "/control");
	active = TST_SYSFS_READ_LI(SMT "/active");

	if (!strcmp(control, "notsupported") ||
	    !strcmp(control, "notimplemented")) {
		if (active == 0) {
			tst_res(TPASS,
				"active is 0 while SMT control is '%s'", control);
		} else {
			tst_res(TFAIL,
				"active is %ld while SMT control is '%s'",
				active, control);
		}
	}
}

static struct tst_test test = {
	.test_all = do_test,
};
