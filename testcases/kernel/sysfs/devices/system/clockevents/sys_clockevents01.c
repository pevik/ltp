// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2026 Cyril Hrubis <chrubis@suse.cz>
 */

/*\
 * Sanity checks for the per-CPU clock event devices exported under
 * /sys/devices/system/clockevents/clockeventN/.
 *
 * The kernel registers one clock event device per online CPU (broadcast is a
 * separate, additional entry used for CPUs whose local timer stops in idle).
 * On platforms without a per-CPU timer, secondary CPUs may instead rely
 * entirely on a broadcast IPI from a single shared timer without ever
 * getting their own clockeventN entry, so the number of clockeventN
 * directories is only asserted not to exceed the number of online CPUs
 * rather than to match it exactly. The test verifies that:
 *
 * - there is at least one clockeventN directory
 * - the number of clockeventN directories does not exceed the number of
 *   online CPUs
 * - each clockeventN/current_device names a non-empty clock event device
 *
 * The test skips with TCONF when no clockevents directory is present.
 */

#include <string.h>
#include <dirent.h>
#include "tst_test.h"
#include "tst_sysfs_assert.h"

#define CLOCKEVENTS "/sys/devices/system/clockevents"
#define SYS_CPU "/sys/devices/system/cpu"

static void check_clockevent(const char *name)
{
	char device[64] = "";

	TST_SYSFS_READ_STR(device, sizeof(device),
			   CLOCKEVENTS "/%s/current_device", name);

	if (device[0] != '\0')
		tst_res(TPASS, "%s/current_device = '%s'", name, device);
	else
		tst_res(TFAIL, "%s/current_device is empty", name);
}

static void do_test(void)
{
	DIR *d;
	struct dirent *ent;
	int nclockevents = 0;
	int online_count, max_id;

	d = SAFE_OPENDIR(CLOCKEVENTS);

	while ((ent = SAFE_READDIR(d))) {
		if (strncmp(ent->d_name, "clockevent", 10))
			continue;

		nclockevents++;
		check_clockevent(ent->d_name);
	}

	SAFE_CLOSEDIR(d);

	if (!nclockevents) {
		tst_res(TCONF, "No clockevent directory found");
		return;
	}

	if (TST_SYSFS_ASSERT_PARSE_LIST(&online_count, &max_id,
					SYS_CPU "/online"))
		return;

	if (nclockevents <= online_count) {
		tst_res(TPASS,
			"number of clockevent devices (%d) does not exceed online CPUs (%d)",
			nclockevents, online_count);
	} else {
		tst_res(TFAIL,
			"number of clockevent devices (%d) exceeds online CPUs (%d)",
			nclockevents, online_count);
	}
}

static struct tst_test test = {
	.test_all = do_test,
};
