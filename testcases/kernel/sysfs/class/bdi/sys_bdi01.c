// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2026 Cyril Hrubis <chrubis@suse.cz>
 */

/*\
 * Sanity checks for the backing device info (BDI) attributes exported under
 * /sys/class/bdi/<id>/.
 *
 * For every BDI the test verifies that:
 *
 * - min_ratio <= max_ratio (both are percentages)
 * - min_ratio_fine <= max_ratio_fine (both are in parts-per-million, i.e.
 *   100% == 1000000)
 * - min_bytes <= max_bytes, when max_bytes is set (0 means "no limit")
 * - stable_pages_required and strict_limit are booleans
 * - read_ahead_kb is non-negative
 *
 * The test skips with TCONF when no BDI is present.
 */

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <dirent.h>
#include <unistd.h>
#include "tst_test.h"
#include "tst_sysfs_assert.h"

#define BDI_CLASS "/sys/class/bdi"

static void check_bdi(const char *id)
{
	long long max_bytes;

	tst_res(TINFO, "Checking bdi '%s'", id);

	TST_SYSFS_ASSERT_RANGELL(0, 100, BDI_CLASS "/%s/min_ratio", id);
	TST_SYSFS_ASSERT_RANGELL(0, 100, BDI_CLASS "/%s/max_ratio", id);
	TST_SYSFS_ASSERT_LE_SUFFIX("min_ratio", "max_ratio",
				   BDI_CLASS "/%s/", id);

	TST_SYSFS_ASSERT_RANGELL(0, 1000000, BDI_CLASS "/%s/min_ratio_fine", id);
	TST_SYSFS_ASSERT_RANGELL(0, 1000000, BDI_CLASS "/%s/max_ratio_fine", id);
	TST_SYSFS_ASSERT_LE_SUFFIX("min_ratio_fine", "max_ratio_fine",
				   BDI_CLASS "/%s/", id);

	max_bytes = TST_SYSFS_READ_LLI(BDI_CLASS "/%s/max_bytes", id);

	if (max_bytes > 0) {
		TST_SYSFS_ASSERT_LE_SUFFIX("min_bytes", "max_bytes",
					   BDI_CLASS "/%s/", id);
	}

	TST_SYSFS_ASSERT_BOOL(BDI_CLASS "/%s/stable_pages_required", id);
	TST_SYSFS_ASSERT_BOOL(BDI_CLASS "/%s/strict_limit", id);

	TST_SYSFS_ASSERT_RANGELL(0, LONG_MAX, BDI_CLASS "/%s/read_ahead_kb", id);
}

static void do_test(void)
{
	DIR *d;
	struct dirent *ent;
	int found = 0;

	if (!tst_sysfs_exists(BDI_CLASS))
		tst_brk(TCONF, BDI_CLASS ": not present");

	d = SAFE_OPENDIR(BDI_CLASS);

	while ((ent = SAFE_READDIR(d))) {
		if (ent->d_name[0] == '.')
			continue;

		found = 1;
		check_bdi(ent->d_name);
	}

	SAFE_CLOSEDIR(d);

	if (!found)
		tst_res(TCONF, "No BDI found");
}

static struct tst_test test = {
	.test_all = do_test,
};
