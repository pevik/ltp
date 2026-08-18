// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2026 Cyril Hrubis <chrubis@suse.cz>
 */

/*\
 * Sanity checks for the wakeup source statistics exported under
 * /sys/class/wakeup/wakeupN/.
 *
 * For every wakeup source the test verifies that:
 *
 * - name is a non-empty string
 * - active_count, active_time_ms, event_count, expire_count, last_change_ms,
 *   max_time_ms, prevent_suspend_time_ms, total_time_ms and wakeup_count are
 *   all non-negative
 * - active_time_ms <= total_time_ms (currently active time is part of the
 *   accumulated total)
 * - max_time_ms <= total_time_ms (the longest single event cannot exceed the
 *   accumulated total)
 *
 * The test skips with TCONF when no wakeup source is present.
 */

#include <string.h>
#include <limits.h>
#include <dirent.h>
#include "tst_test.h"
#include "tst_sysfs_assert.h"

#define WAKEUP "/sys/class/wakeup"

static const char *const nonneg_attrs[] = {
	"active_count",
	"active_time_ms",
	"event_count",
	"expire_count",
	"last_change_ms",
	"max_time_ms",
	"prevent_suspend_time_ms",
	"total_time_ms",
	"wakeup_count",
};

static void check_wakeup(const char *dev)
{
	char name[128] = "";
	unsigned int i;

	tst_res(TINFO, "Checking %s", dev);

	TST_SYSFS_READ_STR(name, sizeof(name), WAKEUP "/%s/name", dev);

	if (name[0] != '\0')
		tst_res(TPASS, "%s: name = '%s'", dev, name);
	else
		tst_res(TFAIL, "%s: name is empty", dev);

	for (i = 0; i < ARRAY_SIZE(nonneg_attrs); i++) {
		TST_SYSFS_ASSERT_RANGELL(0, LONG_MAX, WAKEUP "/%s/%s", dev,
				       nonneg_attrs[i]);
	}

	TST_SYSFS_ASSERT_LE_SUFFIX("active_time_ms", "total_time_ms",
				   WAKEUP "/%s/", dev);
	TST_SYSFS_ASSERT_LE_SUFFIX("max_time_ms", "total_time_ms",
				   WAKEUP "/%s/", dev);
}

static void do_test(void)
{
	DIR *d;
	struct dirent *ent;
	int found = 0;

	d = SAFE_OPENDIR(WAKEUP);

	while ((ent = SAFE_READDIR(d))) {
		if (strncmp(ent->d_name, "wakeup", 6))
			continue;

		found = 1;
		check_wakeup(ent->d_name);
	}

	SAFE_CLOSEDIR(d);

	if (!found)
		tst_res(TCONF, "No wakeup source found");
}

static struct tst_test test = {
	.test_all = do_test,
};
