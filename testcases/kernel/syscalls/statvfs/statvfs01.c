// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) Wipro Technologies Ltd, 2005.  All Rights Reserved.
 *    AUTHOR: Prashant P Yendigeri <prashant.yendigeri@wipro.com>
 * Copyright (c) 2022 SUSE LLC Avinesh Kumar <avinesh.kumar@suse.com>
 */

/*\
 * [Description]
 *
 * Verify that statvfs() executes successfully for all
 * available filesystems.
 */

#include <sys/statvfs.h>
#include "tst_test.h"

#define MNT_POINT "mntpoint"
#define TEST_PATH MNT_POINT"/testfile"

static void run(void)
{
	struct statvfs buf;

	TST_EXP_PASS(statvfs(TEST_PATH, &buf));
}

static void setup(void)
{
	SAFE_TOUCH(TEST_PATH, 0666, NULL);
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.needs_root = 1,
	.mount_device = 1,
	.mntpoint = MNT_POINT,
	.all_filesystems = 1
};
