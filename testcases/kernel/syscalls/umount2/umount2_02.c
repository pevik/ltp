// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2015-2022 FUJITSU LIMITED. All rights reserved
 * Author: Guangwen Feng <fenggw-fnst@cn.fujitsu.com>
 */

/*
 * [Description]
 *
 *  Test for feature MNT_EXPIRE of umount2().
 *
 * - EINVAL when flag is specified with either MNT_FORCE or MNT_DETACH
 * - EAGAIN when initial call to umount2(2) with MNT_EXPIRE
 * - EAGAIN when umount2(2) with MNT_EXPIRE after access(2)
 * - succeed when second call to umount2(2) with MNT_EXPIRE
 *
 *  Test for feature UMOUNT_NOFOLLOW of umount2().
 *
 * - EINVAL when target is a symbolic link
 * - succeed when target is a mount point
 */

#include <sys/mount.h>
#include "lapi/mount.h"
#include "tst_test.h"
#include "umount2.h"

#define MNTPOINT        "mntpoint"
#define SYMLINK	"symlink"

static int mount_flag;

static struct tcase {
	const char *mntpoint;
	int flag;
	int exp_errno;
	int do_access;
	const char *desc;
} tcases[] = {
	{MNTPOINT, MNT_EXPIRE | MNT_FORCE, EINVAL, 0,
		"umount2() with MNT_EXPIRE | MNT_FORCE expected EINVAL"},

	{MNTPOINT, MNT_EXPIRE | MNT_DETACH, EINVAL, 0,
		"umount2() with MNT_EXPIRE | MNT_DETACH expected EINVAL"},

	{MNTPOINT, MNT_EXPIRE, EAGAIN, 0,
		"initial call to umount2() with MNT_EXPIRE expected EAGAIN"},

	{MNTPOINT, MNT_EXPIRE, EAGAIN, 1,
		"umount2() with MNT_EXPIRE after access() expected EAGAIN"},

	{MNTPOINT, MNT_EXPIRE, 0, 0,
		"second call to umount2() with MNT_EXPIRE expected success"},

	{SYMLINK, UMOUNT_NOFOLLOW, EINVAL, 0,
		"umount2('symlink', UMOUNT_NOFOLLOW) expected EINVAL"},

	{MNTPOINT, UMOUNT_NOFOLLOW, 0, 0,
		"umount2('mntpoint', UMOUNT_NOFOLLOW) expected success"},
};

static void test_umount2(unsigned int n)
{
	struct tcase *tc = &tcases[n];

	if (!mount_flag) {
		SAFE_MOUNT(tst_device->dev, MNTPOINT, tst_device->fs_type, 0, NULL);
		mount_flag = 1;
	}

	tst_res(TINFO, "Testing %s", tc->desc);

	if (tc->do_access)
		SAFE_ACCESS(MNTPOINT, F_OK);

	if (tc->exp_errno) {
		TST_EXP_FAIL(umount2_retry(tc->mntpoint, tc->flag), tc->exp_errno,
			"umount2_retry(%s, %d)", tc->mntpoint, tc->flag);
		if (!TST_PASS)
			mount_flag = 0;
	} else {
		TST_EXP_PASS(umount2_retry(tc->mntpoint, tc->flag),
			"umount2_retry(%s, %d)", tc->mntpoint, tc->flag);
		if (TST_PASS)
			mount_flag = 0;
	}
}

static void setup(void)
{
	SAFE_SYMLINK(MNTPOINT, SYMLINK);
}

static void cleanup(void)
{
	if (mount_flag)
		SAFE_UMOUNT(MNTPOINT);
}

static struct tst_test test = {
	.tcnt = ARRAY_SIZE(tcases),
	.cleanup = cleanup,
	.setup = setup,
	.needs_root = 1,
	.needs_device = 1,
	.format_device = 1,
	.mntpoint = MNTPOINT,
	.test = test_umount2,
};
