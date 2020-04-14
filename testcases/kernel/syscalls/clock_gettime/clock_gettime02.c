// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2019 Linaro Limited. All rights reserved.
 * Author: Rafael David Tinoco <rafael.tinoco@linaro.org>
 */
/*
 * Bad argument tests for clock_gettime(2) on multiple clocks:
 *
 *  1) MAX_CLOCKS
 *  2) MAX_CLOCKS + 1
 *  3) CLOCK_REALTIME
 *  4) CLOCK_MONOTONIC
 *  5) CLOCK_PROCESS_CPUTIME_ID
 *  6) CLOCK_THREAD_CPUTIME_ID
 *  7) CLOCK_REALTIME_COARSE
 *  8) CLOCK_MONOTONIC_COARSE
 *  9) CLOCK_MONOTONIC_RAW
 * 10) CLOCK_BOOTTIME
 */

#include "config.h"
#include "tst_safe_clocks.h"
#include "lapi/abisize.h"

#include "clock_gettime.h"

struct test_case {
	clockid_t clktype;
	int exp_err;
	int allow_inval;
};

static struct test_case tc[] = {
	{
	 .clktype = MAX_CLOCKS,
	 .exp_err = EINVAL,
	 },
	{
	 .clktype = MAX_CLOCKS + 1,
	 .exp_err = EINVAL,
	 },
	/*
	 * Different POSIX clocks have different (*clock_get)() handlers.
	 * It justifies testing EFAULT for all.
	 */
	{
	 .clktype = CLOCK_REALTIME,
	 .exp_err = EFAULT,
	 },
	{
	 .clktype = CLOCK_MONOTONIC,
	 .exp_err = EFAULT,
	 },
	{
	 .clktype = CLOCK_PROCESS_CPUTIME_ID,
	 .exp_err = EFAULT,
	 },
	{
	 .clktype = CLOCK_THREAD_CPUTIME_ID,
	 .exp_err = EFAULT,
	 },
	{
	 .clktype = CLOCK_REALTIME_COARSE,
	 .exp_err = EFAULT,
	 .allow_inval = 1,
	 },
	{
	 .clktype = CLOCK_MONOTONIC_COARSE,
	 .exp_err = EFAULT,
	 .allow_inval = 1,
	 },
	{
	 .clktype = CLOCK_MONOTONIC_RAW,
	 .exp_err = EFAULT,
	 .allow_inval = 1,
	 },
	{
	 .clktype = CLOCK_BOOTTIME,
	 .exp_err = EFAULT,
	 .allow_inval = 1,
	 },
};

#ifdef TST_ABI32
static struct timespec spec32;
static struct __kernel_old_timespec kspec32;
#endif

static struct __kernel_timespec kspec64;

/*
 * bad pointer w/ libc causes SIGSEGV signal, call syscall directly
 */
static struct test_variants {
	int (*func)(clockid_t clk_id, void *tp);
	void *spec;
	int spec_size;
	char *desc;
} variants[] = {
#if defined(TST_ABI32)
	{ .func = sys_clock_gettime, .spec = &spec32, .desc = "ABI32 syscall with libc spec"},
	{ .func = sys_clock_gettime, .spec = &kspec32, .desc = "ABI32 syscall with kernel spec"},
#endif

#if defined(TST_ABI64)
	{ .func = sys_clock_gettime, .spec = &kspec64, .desc = "ABI64 syscall with kernel spec"},
#endif

#if (__NR_clock_gettime64 != __LTP__NR_INVALID_SYSCALL)
	{ .func = sys_clock_gettime64, .spec = &kspec64, .desc = "ABI64 syscall time64 with kernel spec"},
#endif
};

static void setup(void)
{
	tst_res(TINFO, "Testing variant: %d: %s", tst_variant, variants[tst_variant].desc);
}

static void verify_clock_gettime(unsigned int i)
{
	struct test_variants *tv = &variants[tst_variant];
	void *specptr;

	/* bad pointer cases */
	if (tc[i].exp_err == EFAULT)
		specptr = tst_get_bad_addr(NULL);
	else
		specptr = tv->spec;

	TEST(tv->func(tc[i].clktype, specptr));

	if (TST_RET != -1) {
		tst_res(TFAIL, "clock_gettime(2): clock %s passed unexcpectedly",
			tst_clock_name(tc[i].clktype));
		return;
	}

	if ((tc[i].exp_err == TST_ERR) ||
	    (tc[i].allow_inval && TST_ERR == EINVAL)) {
		tst_res(TPASS | TTERRNO, "clock_gettime(2): clock %s failed as expected",
			tst_clock_name(tc[i].clktype));
	} else {
		tst_res(TFAIL | TTERRNO, "clock_gettime(2): clock %s failed unexpectedly",
			tst_clock_name(tc[i].clktype));
	}
}

static struct tst_test test = {
	.test = verify_clock_gettime,
	.tcnt = ARRAY_SIZE(tc),
	.test_variants = ARRAY_SIZE(variants),
	.setup = setup,
	.needs_root = 1,
};
