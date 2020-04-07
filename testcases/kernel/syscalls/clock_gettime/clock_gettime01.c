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
#include "tst_timer.h"
#include "tst_safe_clocks.h"
#include "tst_test.h"
#include "lapi/syscalls.h"
#include "lapi/abisize.h"

struct test_case {
	clockid_t clktype;
	int allow_inval;
};

struct test_case tc[] = {
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

struct __kernel_timespec kspec64;

struct timespec spec32;
struct __kernel_old_timespec kspec32;

static int _clock_gettime(clockid_t clk_id, void *tp)
{
	return clock_gettime(clk_id, tp);
}

static int sys_clock_gettime64(clockid_t clk_id, void *tp)
{
	return tst_syscall(__NR_clock_gettime64, clk_id, tp);
}

static int sys_clock_gettime(clockid_t clk_id, void *tp)
{
	return tst_syscall(__NR_clock_gettime, clk_id, tp);
}

struct tmpfunc {
	int (*func)(clockid_t clk_id, void *tp);
	int (*check)(void *spec);
	void *spec;
	int spec_size;
	char *desc;
} variants[] = {
	{ .func = _clock_gettime, .check = tst_timespec_updated_32, .spec = &spec32, .spec_size = sizeof(spec32), .desc = "vDSO or syscall (32)"},
	{ .func = sys_clock_gettime, .check = tst_timespec_updated_32, .spec = &spec32, .spec_size = sizeof(spec32), .desc = "syscall (32) with libc spec"},
	{ .func = sys_clock_gettime, .check = tst_timespec_updated_32, .spec = &kspec32, .spec_size = sizeof(kspec32), .desc = "syscall (32) with kernel spec"},
	{ .func = sys_clock_gettime64, .check = tst_timespec_updated_64, .spec = &kspec64, .spec_size = sizeof(kspec64), .desc = "syscall (64) with kernel spec"},
	{ .func = sys_clock_gettime, .check = tst_timespec_updated_64, .spec = &kspec64, .spec_size = sizeof(kspec64), .desc = "syscall (64) with kernel spec"},
};

static void setup(void)
{
	tst_res(TINFO, "Testing variant: %s", variants[tst_variant].desc);
}

static void verify_clock_gettime(unsigned int i)
{
	struct tmpfunc *tf = &variants[tst_variant];

	memset(tf->spec, 0, tf->spec_size);

	TEST(tf->func(tc[i].clktype, tf->spec));

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
		if (tf->check(tf->spec)) {
			tst_res(TPASS, "clock_gettime(2): clock %s passed",
				tst_clock_name(tc[i].clktype));
		} else {
			tst_res(TFAIL, "clock_gettime(2): clock %s passed, unchanged timespec",
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
