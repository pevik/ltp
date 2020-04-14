// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2019 Linaro Limited. All rights reserved.
 * Author: Rafael David Tinoco <rafael.tinoco@linaro.org>
 */
/*
 * Basic test for clock_gettime(2) on multiple clocks:
 *
 *  1) CLOCK_REALTIME
 *  2) CLOCK_MONOTONIC
 *  3) CLOCK_PROCESS_CPUTIME_ID
 *  4) CLOCK_THREAD_CPUTIME_ID
 *  5) CLOCK_REALTIME_COARSE
 *  6) CLOCK_MONOTONIC_COARSE
 *  7) CLOCK_MONOTONIC_RAW
 *  8) CLOCK_BOOTTIME
 */

#include "config.h"
#include "tst_safe_clocks.h"
#include "lapi/abisize.h"

#include "clock_gettime.h"

struct test_case {
	clockid_t clktype;
	int allow_inval;
};

static struct test_case tc[] = {
	{
	 .clktype = CLOCK_REALTIME,
	 },
	{
	 .clktype = CLOCK_MONOTONIC,
	 },
	{
	 .clktype = CLOCK_PROCESS_CPUTIME_ID,
	 },
	{
	 .clktype = CLOCK_THREAD_CPUTIME_ID,
	 },
	{
	 .clktype = CLOCK_REALTIME_COARSE,
	 .allow_inval = 1,
	 },
	{
	 .clktype = CLOCK_MONOTONIC_COARSE,
	 .allow_inval = 1,
	 },
	{
	 .clktype = CLOCK_MONOTONIC_RAW,
	 .allow_inval = 1,
	 },
	{
	 .clktype = CLOCK_BOOTTIME,
	 .allow_inval = 1,
	 },
};

#ifdef TST_ABI32
static struct timespec spec32;
static struct __kernel_old_timespec kspec32;
#endif

static struct __kernel_timespec kspec64;

static struct test_variants {
	int (*func)(clockid_t clk_id, void *tp);
	int (*check)(void *spec);
	void *spec;
	int spec_size;
	char *desc;
} variants[] = {
#if defined(TST_ABI32)
	{ .func = libc_clock_gettime, .check = tst_timespec_updated_32, .spec = &spec32, .spec_size = sizeof(spec32), .desc = "ABI32 vDSO or syscall"},
	{ .func = sys_clock_gettime, .check = tst_timespec_updated_32, .spec = &spec32, .spec_size = sizeof(spec32), .desc = "ABI32 syscall with libc spec"},
	{ .func = sys_clock_gettime, .check = tst_timespec_updated_32, .spec = &kspec32, .spec_size = sizeof(kspec32), .desc = "ABI32 syscall with kernel spec"},
#endif

#if defined(TST_ABI64)
	{ .func = sys_clock_gettime, .check = tst_timespec_updated_64, .spec = &kspec64, .spec_size = sizeof(kspec64), .desc = "ABI64 syscall with kernel spec"},
#endif

#if (__NR_clock_gettime64 != __LTP__NR_INVALID_SYSCALL)
	{ .func = sys_clock_gettime64, .check = tst_timespec_updated_64, .spec = &kspec64, .spec_size = sizeof(kspec64), .desc = "ABI64 syscall time64 with kernel spec"},
#endif
};

static void setup(void)
{
	tst_res(TINFO, "Testing variant: %s", variants[tst_variant].desc);
}

static void verify_clock_gettime(unsigned int i)
{
	struct test_variants *tv = &variants[tst_variant];
	int ret;

	memset(tv->spec, 0, tv->spec_size);

	TEST(tv->func(tc[i].clktype, tv->spec));

	if (TST_RET == -1) {
		/* errors: allow unsupported clock types */
		if (tc[i].allow_inval && TST_ERR == EINVAL) {
			tst_res(TPASS, "clock_gettime(2): unsupported clock %s failed as expected",
				tst_clock_name(tc[i].clktype));
		} else {
			tst_res(TFAIL | TTERRNO, "clock_gettime(2): clock %s failed unexpectedly",
				tst_clock_name(tc[i].clktype));
		}

	} else {
		/* success: also check if timespec was changed */
		ret = tv->check(tv->spec);
		if (!ret) {
			tst_res(TPASS, "clock_gettime(2): clock %s passed",
				tst_clock_name(tc[i].clktype));
		} else if (ret == -1) {
			tst_res(TFAIL, "clock_gettime(2): clock %s passed, unchanged timespec",
				tst_clock_name(tc[i].clktype));
		} else if (ret == -2) {
			tst_res(TFAIL, "clock_gettime(2): clock %s passed, Corrupted timespec",
				tst_clock_name(tc[i].clktype));
		}
	}
}

static struct tst_test test = {
	.test = verify_clock_gettime,
	.tcnt = ARRAY_SIZE(tc),
	.test_variants = ARRAY_SIZE(variants),
	.setup = setup,
	.needs_root = 1,
};
