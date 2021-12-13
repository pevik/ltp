// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2019 FUJITSU LIMITED. All rights reserved.
 * Author: Yang Xu <xuyang2018.jy@fujitsu.com>
 */

/*\
 * [Description]
 *
 * This is a regresstion test for kernel commit 3dd4d40b4208
 * ("xfs: Sanity check flags of Q_XQUOTARM call").
 */

#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/quota.h>
#include "tst_test.h"
#include "quotactl_syscall_var.h"

#ifdef HAVE_XFS_XQM_H
# include <xfs/xqm.h>


/* Include a valid quota type to avoid other EINVAL error */
static unsigned int invalid_type = XFS_GROUP_QUOTA << 1 | XFS_USER_QUOTA;

static void verify_quota(void)
{
	TEST(do_quotactl(fd, QCMD(Q_XQUOTARM, USRQUOTA), tst_device->dev, 0, (void *)&invalid_type));
	if (TST_ERR == EINVAL)
		tst_res(TPASS, "Q_XQUOTARM has quota type check");
	else
		tst_res(TFAIL, "Q_XQUOTARM doesn't have quota type check");
}

static void setup(void)
{
	quotactl_info();
	fd = SAFE_OPEN(MNTPOINT, O_RDONLY);
}

static void cleanup(void)
{
	if (fd > -1)
		SAFE_CLOSE(fd);
}

static struct tst_test test = {
	.setup = setup,
	.cleanup = cleanup,
	.needs_root = 1,
	.needs_kconfigs = (const char *[]) {
		"CONFIG_XFS_QUOTA",
		NULL
	},
	.test_all = verify_quota,
	.mount_device = 1,
	.dev_fs_type = "xfs",
	.mnt_data = "usrquota",
	.mntpoint = MNTPOINT,
	.test_variants = QUOTACTL_SYSCALL_VARIANTS,
	.tags = (const struct tst_tag[]) {
		{"linux-git", "3dd4d40b4208"},
		{}
	}
};
#else
	TST_TEST_TCONF("System doesn't have <xfs/xqm.h>");
#endif
