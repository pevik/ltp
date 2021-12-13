// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2021 FUJITSU LIMITED. All rights reserved.
 * Author: Yang Xu <xuyang2018.jy@fujitsu.com>
 */

/*\
 * [Description]
 *
 * Tests basic error handling of the quotactl syscall without visible quota files
 * (use quotactl and quotactl_fd syscall):
 *
 * - EFAULT when addr or special is invalid
 * - EINVAL when cmd or type is invalid
 * - ENOTBLK when special is not a block device
 * - ERANGE when cmd is Q_SETQUOTA, but the specified limits are out of the range
 *   allowed by the quota format
 * - EPERM when the caller lacked the required privilege (CAP_SYS_ADMIN) for the
 *   specified operation
 *
 * Minimum e2fsprogs version required is 1.43.
 */

#include <errno.h>
#include <sys/quota.h>
#include "tst_test.h"
#include "tst_capability.h"
#include "quotactl_syscall_var.h"

#define OPTION_INVALID 999

static int32_t fmt_id = QFMT_VFS_V1;
static int test_id, mount_flag;
static int getnextquota_nsup;

static struct if_nextdqblk res_ndq;

static struct dqblk set_dqmax = {
	.dqb_bsoftlimit = 0x7fffffffffffffffLL,  /* 2^63-1 */
	.dqb_valid = QIF_BLIMITS
};

static struct tst_cap dropadmin = {
	.action = TST_CAP_DROP,
	.id = CAP_SYS_ADMIN,
	.name = "CAP_SYS_ADMIN",
};

static struct tst_cap needadmin = {
	.action = TST_CAP_REQ,
	.id = CAP_SYS_ADMIN,
	.name = "CAP_SYS_ADMIN",
};

static struct tcase {
	int cmd;
	int *id;
	void *addr;
	int exp_err;
	int on_flag;
} tcases[] = {
	{QCMD(Q_SETQUOTA, USRQUOTA), &fmt_id, NULL, EFAULT, 1},
	{QCMD(OPTION_INVALID, USRQUOTA), &fmt_id, NULL, EINVAL, 0},
	{QCMD(Q_QUOTAON, USRQUOTA), &fmt_id, NULL, ENOTBLK, 0},
	{QCMD(Q_SETQUOTA, USRQUOTA), &test_id, &set_dqmax, ERANGE, 1},
	{QCMD(Q_QUOTAON, USRQUOTA), &fmt_id, NULL, EPERM, 0},
};

static void verify_quotactl(unsigned int n)
{
	struct tcase *tc = &tcases[n];
	int quota_on = 0;
	int drop_flag = 0;

	if (tc->cmd == QCMD(Q_GETNEXTQUOTA, USRQUOTA) && getnextquota_nsup) {
		tst_res(TCONF, "current system doesn't support Q_GETNEXTQUOTA");
		return;
	}

	if (tc->on_flag) {
		TEST(do_quotactl(fd, QCMD(Q_QUOTAON, USRQUOTA), tst_device->dev,
			fmt_id, NULL));
		if (TST_RET == -1)
			tst_brk(TBROK,
				"quotactl with Q_QUOTAON returned %ld", TST_RET);
		quota_on = 1;
	}

	if (tc->exp_err == EPERM) {
		tst_cap_action(&dropadmin);
		drop_flag = 1;
	}

	if (tst_variant) {
		if (tc->exp_err == ENOTBLK) {
			tst_res(TCONF, "quotactl_fd() doesn't have this error, skip");
			return;
		}
	}
	if (tc->exp_err == ENOTBLK)
		TEST(do_quotactl(fd, tc->cmd, "/dev/null", *tc->id, tc->addr));
	else
		TEST(do_quotactl(fd, tc->cmd, tst_device->dev, *tc->id, tc->addr));

	if (TST_RET == -1) {
		if (tc->exp_err == TST_ERR) {
			tst_res(TPASS | TTERRNO, "quotactl failed as expected");
		} else {
			tst_res(TFAIL | TTERRNO,
				"quotactl failed unexpectedly; expected %s, but got",
				tst_strerrno(tc->exp_err));
		}
	} else {
		tst_res(TFAIL, "quotactl returned wrong value: %ld", TST_RET);
	}

	if (quota_on) {
		TEST(do_quotactl(fd, QCMD(Q_QUOTAOFF, USRQUOTA), tst_device->dev,
			fmt_id, NULL));
		if (TST_RET == -1)
			tst_brk(TBROK,
				"quotactl with Q_QUOTAOFF returned %ld", TST_RET);
		quota_on = 0;
	}

	if (drop_flag) {
		tst_cap_action(&needadmin);
		drop_flag = 0;
	}
}

static void setup(void)
{
	unsigned int i;
	const char *const fs_opts[] = { "-O quota", NULL};

	quotactl_info();
	SAFE_MKFS(tst_device->dev, tst_device->fs_type, fs_opts, NULL);
	SAFE_MOUNT(tst_device->dev, MNTPOINT, tst_device->fs_type, 0, NULL);
	mount_flag = 1;

	fd = SAFE_OPEN(MNTPOINT, O_RDONLY);
	TEST(do_quotactl(fd, QCMD(Q_GETNEXTQUOTA, USRQUOTA), tst_device->dev,
		test_id, (void *) &res_ndq));
	if (TST_ERR == EINVAL || TST_ERR == ENOSYS)
		getnextquota_nsup = 1;

	for (i = 0; i < ARRAY_SIZE(tcases); i++) {
		if (!tcases[i].addr)
			tcases[i].addr = tst_get_bad_addr(NULL);
	}
}

static void cleanup(void)
{
	if (fd > -1)
		SAFE_CLOSE(fd);
	if (mount_flag && tst_umount(MNTPOINT))
		tst_res(TWARN | TERRNO, "umount(%s)", MNTPOINT);
}

static struct tst_test test = {
	.setup = setup,
	.cleanup = cleanup,
	.needs_kconfigs = (const char *[]) {
		"CONFIG_QFMT_V2",
		NULL
	},
	.tcnt = ARRAY_SIZE(tcases),
	.test = verify_quotactl,
	.dev_fs_type = "ext4",
	.mntpoint = MNTPOINT,
	.needs_device = 1,
	.needs_root = 1,
	.test_variants = QUOTACTL_SYSCALL_VARIANTS,
	.needs_cmds = (const char *[]) {
		"mkfs.ext4 >= 1.43.0",
		NULL
	}
};
