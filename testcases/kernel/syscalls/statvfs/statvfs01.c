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
 * available filesystems. Verify statvfs.f_namemax field
 * by trying to create files of valid and invalid length names.
 */

#include <sys/statvfs.h>
#include "tst_test.h"

#define MNT_POINT "mntpoint"
#define TEST_PATH MNT_POINT"/testfile"
#define NLS_MAX_CHARSET_SIZE 6

static void run(void)
{
	struct statvfs buf;
	char *valid_fname, *toolong_fname;
	long fs_type;

	fs_type = tst_fs_type(TEST_PATH);

	TST_EXP_PASS(statvfs(TEST_PATH, &buf));

	toolong_fname = SAFE_MALLOC(buf.f_namemax + 1);
	if (fs_type == TST_VFAT_MAGIC || fs_type == TST_EXFAT_MAGIC)
		valid_fname = SAFE_MALLOC(buf.f_namemax / NLS_MAX_CHARSET_SIZE - 1);
	else
		valid_fname = SAFE_MALLOC(buf.f_namemax - 1);

	memset(toolong_fname, 'b', buf.f_namemax + 1);
	if (fs_type == TST_VFAT_MAGIC || fs_type == TST_EXFAT_MAGIC)
		memset(valid_fname, 'a', buf.f_namemax / NLS_MAX_CHARSET_SIZE - 1);
	else
		memset(valid_fname, 'a', buf.f_namemax - 1);

	TST_EXP_FD(creat(valid_fname, 0444));
	SAFE_CLOSE(TST_RET);

	TST_EXP_FAIL(creat(toolong_fname, 0444), ENAMETOOLONG);
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
