// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2025 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * Verify that `file_getattr` and `file_setattr` are correctly raising
 * EOPNOTSUPP when filesystem doesn't support them.
 *
 * Regression test for "474b155adf39 - fs: make vfs_fileattr_[get|set] return
 * -EOPNOTSUPP".
 */

#include "tst_test.h"
#include "lapi/fs.h"

#define MNTPOINT "mntpoint"
#define FILEPATH (MNTPOINT "/ltp_file")
#define BLOCKS 128
#define PROJID 16

static struct file_attr *attr_set;
static struct file_attr *attr_get;

static void run(void)
{
	TST_EXP_FAIL(file_setattr(AT_FDCWD, FILEPATH,
			   attr_set, FILE_ATTR_SIZE_LATEST, 0), EOPNOTSUPP);

	TST_EXP_FAIL(file_getattr(AT_FDCWD, FILEPATH,
			   attr_get, FILE_ATTR_SIZE_LATEST, 0), EOPNOTSUPP);
}

static void setup(void)
{
	struct stat statbuf;

	SAFE_TOUCH(FILEPATH, 0777, NULL);

	SAFE_STAT(MNTPOINT, &statbuf);

	attr_set->fa_xflags |= FS_XFLAG_EXTSIZE;
	attr_set->fa_xflags |= FS_XFLAG_COWEXTSIZE;
	attr_set->fa_extsize = BLOCKS * statbuf.st_blksize;
	attr_set->fa_cowextsize = BLOCKS * statbuf.st_blksize;
	attr_set->fa_projid = PROJID;
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.mntpoint = MNTPOINT,
	.needs_root = 1,
	.mount_device = 1,
	.filesystems = (struct tst_fs []) {
		{.type = "vfat"},
		{}
	},
	.bufs = (struct tst_buffers []) {
		{&attr_set, .size = sizeof(struct file_attr)},
		{&attr_get, .size = sizeof(struct file_attr)},
		{}
	}
};
