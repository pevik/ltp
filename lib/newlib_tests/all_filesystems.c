// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2022 Petr Vorel <pvorel@suse.cz>
 */

#include "tst_test.h"

#define MOUNT_PATH "mntpoint"

static void do_test(void)
{
	long f_type = tst_fs_type(MOUNT_PATH);
	const char *fs = tst_fs_type_name(f_type);

	switch (f_type) {
	/* tst_fs_type() returns underlying system, i.e "ntfs" */
	case TST_FUSE_MAGIC:
	/* tst_fs_type_name() returns "ext2/ext3/ext4" */
	case TST_EXT234_MAGIC:
		TST_EXP_PASS(!strcmp(tst_device->fs_type, fs));
		break;
	default:
		TST_EXP_PASS(strcmp(tst_device->fs_type, fs));
	}
}

static struct tst_test test = {
	.test_all = do_test,
	.needs_root = 1,
	.mount_device = 1,
	.mntpoint = MOUNT_PATH,
	.all_filesystems = 1,
};
