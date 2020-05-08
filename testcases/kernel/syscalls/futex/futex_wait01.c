// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2015 Cyril Hrubis <chrubis@suse.cz>
 *
 * Based on futextest (futext_wait_timeout.c and futex_wait_ewouldblock.c)
 * written by Darren Hart <dvhltc@us.ibm.com>
 *            Gowrishankar <gowrishankar.m@in.ibm.com>
 *
 * 1. Block on a futex and wait for timeout.
 * 2. Test if FUTEX_WAIT op returns -EWOULDBLOCK if the futex value differs
 *    from the expected one.
 */

#include <errno.h>

#include "futextest.h"
#include "lapi/abisize.h"

struct testcase {
	futex_t *f_addr;
	futex_t f_val;
	int opflags;
	int exp_errno;
};

static futex_t futex = FUTEX_INITIALIZER;

static struct testcase testcases[] = {
	{&futex, FUTEX_INITIALIZER, 0, ETIMEDOUT},
	{&futex, FUTEX_INITIALIZER+1, 0, EWOULDBLOCK},
	{&futex, FUTEX_INITIALIZER, FUTEX_PRIVATE_FLAG, ETIMEDOUT},
	{&futex, FUTEX_INITIALIZER+1, FUTEX_PRIVATE_FLAG, EWOULDBLOCK},
};

static struct test_variants {
	enum futex_fn_type fntype;
	enum tst_ts_type tstype;
	char *desc;
} variants[] = {
#if defined(TST_ABI32)
	{ .fntype = FUTEX_FN_FUTEX, .tstype = TST_KERN_OLD_TIMESPEC, .desc = "syscall with kernel spec32"},
#endif

#if defined(TST_ABI64)
	{ .fntype = FUTEX_FN_FUTEX, .tstype = TST_KERN_TIMESPEC, .desc = "syscall with kernel spec64"},
#endif

#if (__NR_futex_time64 != __LTP__NR_INVALID_SYSCALL)
	{ .fntype = FUTEX_FN_FUTEX64, .tstype = TST_KERN_TIMESPEC, .desc = "syscall time64 with kernel spec64"},
#endif
};

static void run(unsigned int n)
{
	struct test_variants *tv = &variants[tst_variant];
	struct testcase *tc = &testcases[n];
	static struct tst_ts to;
	int res;

	to.type = tv->tstype;
	tst_ts_set_sec(&to, 0);
	tst_ts_set_nsec(&to, 10000);

	res = futex_wait(tv->fntype, tc->f_addr, tc->f_val, &to, tc->opflags);

	if (res != -1) {
		tst_res(TFAIL, "futex_wait() succeeded unexpectedly");
		return;
	}

	if (errno != tc->exp_errno) {
		tst_res(TFAIL | TTERRNO, "futex_wait() failed with incorrect error, expected errno=%s",
		         tst_strerrno(tc->exp_errno));
		return;
	}

	tst_res(TPASS | TERRNO, "futex_wait() passed");
}

static void setup(void)
{
	tst_res(TINFO, "Testing variant: %s", variants[tst_variant].desc);
}

static struct tst_test test = {
	.setup = setup,
	.test = run,
	.tcnt = ARRAY_SIZE(testcases),
	.test_variants = ARRAY_SIZE(variants),
};
