// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  Copyright (c) Linux Test Project, 2020
 *  Copyright (c) Wipro Technologies Ltd, 2002
 */

/*
 * This is an error test for iopl(2) system call.
 *
 * Verify that
 *  1) iopl(2) returns -1 and sets errno to EINVAL for privilege
 *     level greater than 3.
 *  2) iopl(2) returns -1 and sets errno to EPERM if the current
 *     user is not the super-user.
 *
 * Author: Subhab Biswas <subhabrata.biswas@wipro.com>
 */

#include <errno.h>
#include <unistd.h>
#include <sys/io.h>
#include <pwd.h>
#include "tst_test.h"
#include "tst_safe_macros.h"

#if defined __i386__ || defined(__x86_64__)

#define INVALID_LEVEL 4		/* Invalid privilege level */
#define EXP_RET_VAL -1

static struct tcase {
	int level;	/* I/O privilege level */
	char *desc;	/* test case description */
	int exp_errno;	/* expected error number */
} tcases[] = {
	{INVALID_LEVEL, "Invalid privilege level", EINVAL},
	{1, "Non super-user", EPERM}
};

static void verify_iopl(unsigned int i)
{
	if (i == 1) {
		/* set Non super-user for second test */
		struct passwd *pw;
		pw = SAFE_GETPWNAM("nobody");
		if (seteuid(pw->pw_uid) == -1) {
			tst_res(TWARN, "Failed to set effective"
					"uid to %d", pw->pw_uid);
			return;
		}
	}

	TEST(iopl(tcases[i].level));

	if ((TST_RET == EXP_RET_VAL) && (TST_ERR == tcases[i].exp_errno)) {
		tst_res(TPASS, "Expected failure for %s, "
				"errno: %d", tcases[i].desc,
				TST_ERR);
	} else {
		tst_res(TFAIL, "Unexpected results for %s ; "
				"returned %ld (expected %d), errno %d "
				"(expected errno  %d)",
				tcases[i].desc,
				TST_RET, EXP_RET_VAL,
				TST_ERR, tcases[i].exp_errno);
	}

	if (i == 1) {
		/* revert back to super user */
		SAFE_SETEUID(0);
	}
}

static struct tst_test test = {
	.tcnt = ARRAY_SIZE(tcases),
	.test = verify_iopl,
	.needs_root = 1,
};

#else /* __i386__, __x86_64__*/
TST_TEST_TCONF("LSB v1.3 does not specify iopl() for this architecture. (only for i386 or x86_64)");
#endif /* __i386_, __x86_64__*/
