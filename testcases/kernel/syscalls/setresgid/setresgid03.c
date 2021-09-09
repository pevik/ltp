// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Wipro Technologies Ltd, 2002.  All Rights Reserved.
 *    AUTHOR: Madhu T L <madhu.tarikere@wipro.com>
 * Copyright (C) 2021 SUSE LLC <mdoucha@suse.cz>
 */

/*\
 * [Description]
 *
 * Verify that setresgid() fails with EPERM if unprivileged user tries to set
 * process group ID which requires higher permissions.
 */

#include <sys/types.h>
#include <pwd.h>

#include "tst_test.h"
#include "tst_uid.h"
#include "compat_tst_16.h"

struct test_case_t {		/* test case structure */
	gid_t *rgid;		/* real GID */
	gid_t *egid;		/* effective GID */
	gid_t *sgid;		/* saved GID */
	gid_t *exp_rgid;	/* Expected real GID */
	gid_t *exp_egid;	/* Expected effective GID */
	gid_t *exp_sgid;	/* Expected saved GID */
	char *desc;		/* Test description */
};

static gid_t nobody_gid, other_gid, neg = -1;

static struct test_case_t test_cases[] = {
	{&neg, &neg, &other_gid, &nobody_gid, &nobody_gid, &nobody_gid,
		"setresgid(-1, -1, other)"},
	{&neg, &other_gid, &neg, &nobody_gid, &nobody_gid, &nobody_gid,
		"setresgid(-1, other, -1)"},
	{&other_gid, &neg, &neg, &nobody_gid, &nobody_gid, &nobody_gid,
		"setresgid(other, -1, -1)"},
	{&other_gid, &other_gid, &other_gid, &nobody_gid, &nobody_gid,
		&nobody_gid, "setresgid(other, other, other)"},
};

static void setup(void)
{
	gid_t test_groups[2];
	struct passwd *pw = SAFE_GETPWNAM("nobody");

	nobody_gid = test_groups[0] = pw->pw_gid;
	tst_get_gids(test_groups, 1, 2);
	other_gid = test_groups[1];

	GID16_CHECK(nobody_gid, setresgid);
	GID16_CHECK(other_gid, setresgid);

	/* Set real/effective/saved gid to nobody */
	SAFE_SETRESGID(nobody_gid, nobody_gid, nobody_gid);
	SAFE_SETUID(pw->pw_uid);
}

static void run(unsigned int n)
{
	const struct test_case_t *tc = test_cases + n;

	TEST(SETRESGID(*tc->rgid, *tc->egid, *tc->sgid));

	if (TST_RET != -1) {
		tst_res(TFAIL | TTERRNO, "%s returned unexpected value %ld",
			tc->desc, TST_RET);
		return;
	}

	if (TST_ERR != EPERM) {
		tst_res(TFAIL | TTERRNO, "%s returned unexpected error",
			tc->desc);
		return;
	}

	if (tst_check_resgid(tc->desc, *tc->exp_rgid, *tc->exp_egid,
		*tc->exp_sgid))
		tst_res(TPASS | TTERRNO, "%s failed as expected", tc->desc);
}

static struct tst_test test = {
	.test = run,
	.tcnt = ARRAY_SIZE(test_cases),
	.setup = setup,
	.needs_root = 1,
};
