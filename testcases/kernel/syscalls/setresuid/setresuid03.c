// SPDX-License-Identifier: GPL-2.0-only
/*
 *   Copyright (c) International Business Machines  Corp., 2001
 *	07/2001 ported by John George
 * Copyright (C) 2021 SUSE LLC <mdoucha@suse.cz>
 */

/*\
 * [Description]
 *
 * Test that the setresuid system call sets the proper errno values when
 * a non-root user attempts to change the real, effective or saved uid
 * to a value other than one of the current uid, the current effective uid
 * or the current saved uid.
 */

#include "tst_test.h"
#include "tst_uid.h"
#include "compat_tst_16.h"

static uid_t root_uid, main_uid, other_uid, neg_one = -1;

static struct test_data_t {
	uid_t *real_uid;
	uid_t *eff_uid;
	uid_t *sav_uid;
	int exp_errno;
	uid_t *exp_real_uid;
	uid_t *exp_eff_uid;
	uid_t *exp_sav_uid;
	char *test_msg;
} test_data[] = {
	{&other_uid, &neg_one, &neg_one, EPERM, &root_uid, &main_uid,
		&main_uid, "setresuid(other, -1, -1)"},
	{&neg_one, &neg_one, &other_uid, EPERM, &root_uid, &main_uid,
		&main_uid, "setresuid(-1, -1, other)"},
	{&neg_one, &other_uid, &neg_one, EPERM, &root_uid, &main_uid,
		&main_uid, "setresuid(-1, other, -1)"}
};

static void setup(void)
{
	uid_t test_users[2];

	root_uid = getuid();
	tst_get_uids(test_users, 0, 2);
	main_uid = test_users[0];
	other_uid = test_users[1];

	UID16_CHECK(root_uid, setresuid);
	UID16_CHECK(main_uid, setresuid);
	UID16_CHECK(other_uid, setresuid);

	SAFE_SETRESUID(root_uid, main_uid, main_uid);
}

static void run(unsigned int n)
{
	const struct test_data_t *tc = test_data + n;

	TEST(SETRESUID(*tc->real_uid, *tc->eff_uid, *tc->sav_uid));

	if (TST_RET != -1) {
		tst_res(TFAIL | TTERRNO, "%s returned unexpected value %ld",
			tc->test_msg, TST_RET);
		return;
	}

	if (TST_ERR != tc->exp_errno) {
		tst_res(TFAIL | TTERRNO, "%s returned unexpected error",
			tc->test_msg);
		return;
	}

	if (tst_check_resuid(tc->test_msg, *tc->exp_real_uid, *tc->exp_eff_uid,
		*tc->exp_sav_uid))
		tst_res(TPASS | TTERRNO, "%s failed as expected", tc->test_msg);
}

static struct tst_test test = {
	.test = run,
	.tcnt = ARRAY_SIZE(test_data),
	.setup = setup,
	.needs_root = 1,
};
