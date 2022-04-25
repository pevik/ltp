// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2022 FUJITSU LIMITED. All rights reserved.
 * Author: Dai Shili <daisl.fnst@fujitsu.com>
 * Author: Chen Hanxiao <chenhx.fnst@fujitsu.com>
 */

/*\
 * [Description]
 *
 * Basic mount_setattr() test.
 * Test whether the basic mount attributes are set correctly.
 *
 * Verify some MOUNT_SETATTR(2) attributes:
 *
 * - 1) MOUNT_ATTR_RDONLY - makes the mount read-only
 * - 2) MOUNT_ATTR_NOSUID - causes the mount not to honor the
 *     set-user-ID and set-group-ID mode bits and file capabilities
 *     when executing programs.
 * - 3) MOUNT_ATTR_NODEV - prevents access to devices on this mount
 * - 4) MOUNT_ATTR_NOEXEC - prevents executing programs on this mount
 * - 5) MOUNT_ATTR_NOSYMFOLLOW - prevents following symbolic links
 *    on this mount
 * - 6) MOUNT_ATTR_NODIRATIME - prevents updating access time for
 *    directories on this mount
 *
 * The functionality was added in v5.12.
 */

#define _GNU_SOURCE

#include <sys/statvfs.h>
#include "tst_test.h"
#include "lapi/fsmount.h"
#include "lapi/stat.h"

#define MNTPOINT        "mntpoint"
#define OT_MNTPOINT     "ot_mntpoint"
#define TCASE_ENTRY(attrs, exp_attrs)   \
	{                                \
		.name = #attrs,                 \
		.mount_attrs = attrs,           \
		.expect_attrs = exp_attrs       \
	}

static int dir_created;
static int mount_flag, otfd;

static struct tcase {
	char *name;
	unsigned int mount_attrs;
	unsigned int expect_attrs;
} tcases[] = {
	TCASE_ENTRY(MOUNT_ATTR_RDONLY, ST_RDONLY),
	TCASE_ENTRY(MOUNT_ATTR_NOSUID, ST_NOSUID),
	TCASE_ENTRY(MOUNT_ATTR_NODEV, ST_NODEV),
	TCASE_ENTRY(MOUNT_ATTR_NOEXEC, ST_NOEXEC),
	TCASE_ENTRY(MOUNT_ATTR_NOSYMFOLLOW, ST_NOSYMFOLLOW),
	TCASE_ENTRY(MOUNT_ATTR_NODIRATIME, ST_NODIRATIME),
};

static void cleanup(void)
{
	if (otfd > -1)
		SAFE_CLOSE(otfd);
	if (mount_flag && tst_is_mounted_at_tmpdir(OT_MNTPOINT)) {
		mount_flag = 0;
		SAFE_UMOUNT(OT_MNTPOINT);
	}
	if (dir_created)
		SAFE_RMDIR(OT_MNTPOINT);
}

static void setup(void)
{
	fsopen_supported_by_kernel();
	SAFE_MKDIR(OT_MNTPOINT, 0777);
	dir_created = 1;
}

static void run(unsigned int n)
{
	struct tcase *tc = &tcases[n];
	struct mount_attr attr = {
		.attr_set = tc->mount_attrs,
	};
	struct statvfs buf;

	TST_EXP_FD_SILENT(open_tree(AT_FDCWD, MNTPOINT, AT_EMPTY_PATH |
		AT_SYMLINK_NOFOLLOW | OPEN_TREE_CLOEXEC | OPEN_TREE_CLONE));
	if (!TST_PASS)
		goto out;

	otfd = (int)TST_RET;

	TST_EXP_PASS_SILENT(mount_setattr(otfd, "", AT_EMPTY_PATH, &attr, sizeof(attr)),
		"%s set", tc->name);
	if (!TST_PASS)
		return;

	TST_EXP_PASS_SILENT(move_mount(otfd, "", AT_FDCWD, OT_MNTPOINT, MOVE_MOUNT_F_EMPTY_PATH));
	if (!TST_PASS)
		goto out;
	mount_flag = 1;

	TST_EXP_PASS_SILENT(statvfs(OT_MNTPOINT, &buf), "statvfs sucess");
	if (!TST_PASS)
		goto out;

	if (buf.f_flag & tc->expect_attrs)
		tst_res(TPASS, "%s is actually set as expected", tc->name);
	else
		tst_res(TFAIL, "%s is not actually set as expected", tc->name);

out:
	if (otfd > -1)
		SAFE_CLOSE(otfd);

	if (tst_is_mounted_at_tmpdir(OT_MNTPOINT)) {
		mount_flag = 0;
		SAFE_UMOUNT(OT_MNTPOINT);
	}
}

static struct tst_test test = {
	.tcnt = ARRAY_SIZE(tcases),
	.test = run,
	.setup = setup,
	.cleanup = cleanup,
	.needs_root = 1,
	.mount_device = 1,
	.mntpoint = MNTPOINT,
	.all_filesystems = 1,
	.skip_filesystems = (const char *const []){"fuse", NULL},
};
