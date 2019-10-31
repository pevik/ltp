// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2013 Fujitsu Ltd.
 * Author: DAN LI <li.dan@cn.fujitsu.com>
 */

/*
 * Test Name: quotactl02
 *
 * Description:
 * This testcase checks basic flags of quotactl(2) for an XFS file system:
 * 1) quotactl(2) succeeds to turn off xfs quota and get xfs quota off status.
 * 2) quotactl(2) succeeds to turn on xfs quota and get xfs quota on status.
 * 3) quotactl(2) succeeds to set and use Q_XGETQUOTA to get xfs disk quota
 *    limits.
 * 4) quotactl(2) succeeds to set and use Q_XGETNEXTQUOTA to get xfs disk
 *    quota limits.
 */
#define _GNU_SOURCE
#include "config.h"
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/quota.h>

#include "tst_test.h"
#include "lapi/quotactl.h"

#ifdef HAVE_XFS_XQM_H
#include <xfs/xqm.h>

static void check_qoff(int, char *);
static void check_qon(int, char *);
static void check_qlim(int, char *);

static uint32_t test_id;
static struct fs_disk_quota set_dquota = {
	.d_rtb_softlimit = 1000,
	.d_fieldmask = FS_DQ_RTBSOFT
};
static uint32_t qflag = XFS_QUOTA_UDQ_ENFD;
static const char mntpoint[] = "mnt_point";

static struct t_case {
	int cmd;
	void *addr;
	void (*func_check)();
	int check_subcmd;
	char *des;
} tcases[] = {
	{QCMD(Q_XQUOTAOFF, USRQUOTA), &qflag, check_qoff, Q_XGETQSTAT,
	"turn off xfs quota and get xfs quota off status"},
	{QCMD(Q_XQUOTAON, USRQUOTA), &qflag, check_qon, Q_XGETQSTAT,
	"turn on xfs quota and get xfs quota on status"},
	{QCMD(Q_XSETQLIM, USRQUOTA), &set_dquota, check_qlim, Q_XGETQUOTA,
	"Q_XGETQUOTA"},
	{QCMD(Q_XSETQLIM, USRQUOTA), &set_dquota, check_qlim, Q_XGETNEXTQUOTA,
	"Q_XGETNEXTQUOTA"},
};

static void check_qoff(int subcmd, char *desp)
{
	int res;
	struct fs_quota_stat res_qstat;

	res = quotactl(QCMD(subcmd, USRQUOTA), tst_device->dev,
	               test_id, (void*) &res_qstat);
	if (res == -1) {
		tst_res(TFAIL | TERRNO,
			"quotactl() failed to get xfs quota off status");
		return;
	}

	if (res_qstat.qs_flags & XFS_QUOTA_UDQ_ENFD) {
		tst_res(TFAIL, "xfs quota enforcement was on unexpectedly");
		return;
	}

	tst_res(TPASS, "quoactl() succeeded to %s", desp);
}

static void check_qon(int subcmd, char *desp)
{
	int res;
	struct fs_quota_stat res_qstat;

	res = quotactl(QCMD(subcmd, USRQUOTA), tst_device->dev,
	               test_id, (void*) &res_qstat);
	if (res == -1) {
		tst_res(TFAIL | TERRNO,
			"quotactl() failed to get xfs quota on status");
		return;
	}

	if (!(res_qstat.qs_flags & XFS_QUOTA_UDQ_ENFD)) {
		tst_res(TFAIL, "xfs quota enforcement was off unexpectedly");
		return;
	}

	tst_res(TPASS, "quoactl() succeeded to %s", desp);
}

static void check_qlim(int subcmd, char *desp)
{
	int res;
	static struct fs_disk_quota res_dquota;

	res_dquota.d_rtb_softlimit = 0;

	res = quotactl(QCMD(subcmd, USRQUOTA), tst_device->dev,
	               test_id, (void*) &res_dquota);
	if (res == -1) {
		if (errno == EINVAL) {
			tst_brk(TCONF | TERRNO,
				"%s wasn't supported in quotactl()", desp);
		}
		tst_res(TFAIL | TERRNO,
			"quotactl() failed to get xfs disk quota limits");
		return;
	}

	if (res_dquota.d_id != test_id) {
		tst_res(TFAIL, "quotactl() got unexpected user id %u,"
			" expected %u", res_dquota.d_id, test_id);
		return;
	}

	if (res_dquota.d_rtb_hardlimit != set_dquota.d_rtb_hardlimit) {
		tst_res(TFAIL, "quotactl() got unexpected rtb soft limit %llu,"
			" expected %llu", res_dquota.d_rtb_hardlimit,
			set_dquota.d_rtb_hardlimit);
		return;
	}

	tst_res(TPASS, "quoactl() succeeded to set and use %s to get xfs disk "
		"quota limits", desp);
}

static void setup(void)
{
	test_id = geteuid();
}

static void verify_quota(unsigned int n)
{
	struct t_case *tc = &tcases[n];

	TEST(quotactl(tc->cmd, tst_device->dev, test_id, tc->addr));
	if (TST_RET == -1) {
		tst_res(TFAIL | TTERRNO, "quotactl() failed to %s", tc->des);
		return;
	}

	tc->func_check(tc->check_subcmd, tc->des);
}

static const char *kconfigs[] = {
	"CONFIG_XFS_QUOTA",
	NULL
};

static struct tst_test test = {
	.needs_tmpdir = 1,
	.needs_root = 1,
	.needs_kconfigs = kconfigs,
	.test = verify_quota,
	.tcnt = ARRAY_SIZE(tcases),
	.mount_device = 1,
	.dev_fs_type = "xfs",
	.mntpoint = mntpoint,
	.mnt_data = "usrquota",
	.setup = setup,
};
#else
	TST_TEST_TCONF("System doesn't have <xfs/xqm.h>");
#endif
