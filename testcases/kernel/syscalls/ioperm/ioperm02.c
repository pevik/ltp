// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  Copyright (c) Linux Test Project, 2020
 *  Copyright (c) Wipro Technologies Ltd, 2002
 */

/*
 * This is an error test for ioperm(2) system call.
 *
 * Verify that
 * 1) ioperm(2) returns -1 and sets errno to EINVAL for I/O port
 *    address greater than 0x3ff.
 * 2) ioperm(2) returns -1 and sets errno to EPERM if the current
 *    user is not the super-user.
 *
 * Author: Subhab Biswas <subhabrata.biswas@wipro.com>
 */

#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/io.h>
#include <pwd.h>
#include "tst_test.h"
#include "tst_safe_macros.h"

#if defined __i386__ || defined(__x86_64__)

#define NUM_BYTES 3
#define TURN_ON 1
#define TURN_OFF 0
#define EXP_RET_VAL -1
#ifndef IO_BITMAP_BITS
#define IO_BITMAP_BITS 1024	/* set to default value since some H/W may not support 0x10000 even with a 2.6.8 kernel */
#define IO_BITMAP_BITS_16 65536
#endif

static struct tcase_t {
	long from;	/* starting port address */
	long num;	/* no. of bytes from starting address */
	int turn_on;
	char *desc;	/* test case description */
	int exp_errno;	/* expected error number */
} tcases[] = {
	{0, NUM_BYTES, TURN_ON, "Invalid I/O address", EINVAL},
	{0, NUM_BYTES, TURN_ON, "Non super-user", EPERM},
};

static void setup(void)
{
	/*
	 * The value of IO_BITMAP_BITS (include/asm-i386/processor.h) changed
	 * from kernel 2.6.8 to permit 16-bits (65536) ioperm
	 *
	 * Ricky Ng-Adam, rngadam@yahoo.com
	 */
	if ((tst_kvercmp(2, 6, 8) < 0) || (tst_kvercmp(2, 6, 9) == 0)) {
		/*try invalid ioperm on 1022, 1023, 1024 */
		tcases[0].from = (IO_BITMAP_BITS - NUM_BYTES) + 1;

		/*try get valid ioperm on 1021, 1022, 1023 */
		tcases[1].from = IO_BITMAP_BITS - NUM_BYTES;
	} else {
		/*try invalid ioperm on 65534, 65535, 65536 */
		tcases[0].from = (IO_BITMAP_BITS_16 - NUM_BYTES) + 1;

		/*try valid ioperm on 65533, 65534, 65535 */
		tcases[1].from = IO_BITMAP_BITS_16 - NUM_BYTES;
	}

}

static void verify_ioperm(unsigned int i)
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

	TEST(ioperm(tcases[i].from, tcases[i].num, tcases[i].turn_on));

	if ((TST_RET == EXP_RET_VAL) && (TST_ERR == tcases[i].exp_errno)) {
		tst_res(TPASS, "Expected failure for %s, "
				"errno: %d", tcases[i].desc, TST_ERR);
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
	.test = verify_ioperm,
	.needs_root = 1,
	.setup = setup,
};

#else /* __i386__, __x86_64__*/
TST_TEST_TCONF("LSB v1.3 does not specify ioperm() for this architecture. (only for i386 or x86_64)");
#endif /* __i386_, __x86_64__*/
