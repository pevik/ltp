// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) International Business Machines  Corp., 2001
 * Ported by Wayne Boyer
 * Adapted by Dustin Kirkland (k1rkland@us.ibm.com)
 * Adapted by Zhao gongyi (zhaogongyi@huawei.com)
 */

/*\
 * [Description]
 * Testcase to check the basic functionality of setfsgid(2) system
 * call failures with priviledged or unpriviledged user.
 */

#include <pwd.h>
#include "tst_test.h"
#include "compat_tst_16.h"

static gid_t gid;
static gid_t pre_gid;
static const char nobody_uid[] = "nobody";
static struct passwd *ltpuser;

static void run(unsigned int i)
{
	int cnt;

	GID16_CHECK(gid, setfsgid);

	if (i == 0) {
		ltpuser = SAFE_GETPWNAM(nobody_uid);
		SAFE_SETEUID(ltpuser->pw_uid);
	}

	/*
	 * Run SETFSGID() twice to check the second running have changed
	 * the gid for priviledged user, and have not changed the gid
	 * for unpriviledged user.
	 */
	for (cnt = 0; cnt < 2; cnt++) {
		TEST(SETFSGID(gid));
		if (pre_gid != TST_RET) {
			tst_res(TFAIL, "setfsgid() returned %ld", TST_RET);
		} else {
			tst_res(TPASS, "setfsgid() returned expected value: %ld",
				TST_RET);
			if (i == 1) {
				pre_gid = gid;
				gid++;
			}
		}
	}

	if (i == 0)
		SAFE_SETEUID(0);
}

static void setup(void)
{
	pre_gid = 0;
	gid = 1;
}

static struct tst_test test = {
	.needs_root = 1,
	.setup = setup,
	.test = run,
	.tcnt = 2,
};
