// SPDX-License-Identifier: GPL-2.0-or-later
/*

  Copyright (c) 2020 Cyril Hrubis <chrubis@suse.cz>

 */
/*

  Basic test for timer namespaces.

  After a call to unshare(CLONE_NEWTIME) a new timer namespace is created, the
  process that has called the unshare() can adjust offsets for CLOCK_MONOTONIC
  and CLOCK_BOOTTIME for its children by writing to the '/proc/self/timens_offsets'.

  The child processes also switch to the initial parent namespace and checks
  that the offset is set to 0.

 */

#define _GNU_SOURCE
#include "tst_safe_clocks.h"
#include "lapi/namespaces_constants.h"
#include "lapi/abisize.h"

#include "clock_gettime.h"

static inline long long timespec_diff_ms(void *t1, void *t2)
{
	struct timespec *ts1 = t1, *ts2 = t2;

	return tst_timespec_diff_ms(*ts1, *ts2);
}

static inline long long timespec64_diff_ms(void *t1, void *t2)
{
	struct __kernel_timespec *ts1 = t1, *ts2 = t2;

	return tst_timespec64_diff_ms(*ts1, *ts2);
}

static struct tcase {
	int clk_id;
	int clk_off;
	int off;
} tcases[] = {
	{CLOCK_MONOTONIC, CLOCK_MONOTONIC, 10},
	{CLOCK_BOOTTIME, CLOCK_BOOTTIME, 10},

	{CLOCK_MONOTONIC, CLOCK_MONOTONIC, -10},
	{CLOCK_BOOTTIME, CLOCK_BOOTTIME, -10},

	{CLOCK_MONOTONIC_RAW, CLOCK_MONOTONIC, 100},
	{CLOCK_MONOTONIC_COARSE, CLOCK_MONOTONIC, 100},
};

static int parent_ns;

#ifdef TST_ABI32
static struct timespec spec32_now, spec32_then, spec32_pthen;
static struct __kernel_old_timespec kspec32_now, kspec32_then, kspec32_pthen;
#endif

static struct __kernel_timespec kspec64_now, kspec64_then, kspec64_pthen;

static struct test_variants {
	int (*func)(clockid_t clk_id, void *tp);
	long long (*diff)(void *t1, void *t2);
	void *now;
	void *then;
	void *pthen;
	int spec_size;
	char *desc;
} variants[] = {
#if defined(TST_ABI32)
	{ .func = libc_clock_gettime, .diff = timespec_diff_ms, .now = &spec32_now, .then = &spec32_then, .pthen = &spec32_pthen, .desc = "ABI32 vDSO or syscall"},
	{ .func = sys_clock_gettime, .diff = timespec_diff_ms, .now = &spec32_now, .then = &spec32_then, .pthen = &spec32_pthen, .desc = "ABI32 syscall with libc spec"},
	{ .func = sys_clock_gettime, .diff = timespec_diff_ms, .now = &kspec32_now, .then = &kspec32_then, .pthen = &kspec32_pthen, .desc = "ABI32 syscall with kernel spec"},
#endif

#if defined(TST_ABI64)
	{ .func = sys_clock_gettime, .diff = timespec64_diff_ms, .now = &kspec64_now, .then = &kspec64_then, .pthen = &kspec64_pthen, .desc = "ABI64 syscall with kernel spec"},
#endif

#if (__NR_clock_gettime64 != __LTP__NR_INVALID_SYSCALL)
	{ .func = sys_clock_gettime64, .diff = timespec64_diff_ms, .now = &kspec64_now, .then = &kspec64_then, .pthen = &kspec64_pthen, .desc = "ABI64 syscall time64 with kernel spec"},
#endif
};

static void child(struct test_variants *tv, struct tcase *tc)
{
	long long diff;

	if (tv->func(tc->clk_id, tv->then)) {
		tst_brk(TBROK | TERRNO, "%d clock_gettime(%s) failed",
			__LINE__, tst_clock_name(tc->clk_id));
	}

	SAFE_SETNS(parent_ns, CLONE_NEWTIME);

	if (tv->func(tc->clk_id, tv->pthen)) {
		tst_brk(TBROK | TERRNO, "%d clock_gettime(%s) failed",
			__LINE__, tst_clock_name(tc->clk_id));
	}

	diff = tv->diff(tv->then, tv->now);

	if (diff/1000 != tc->off) {
		tst_res(TFAIL, "Wrong offset (%s) read %llims",
		        tst_clock_name(tc->clk_id), diff);
	} else {
		tst_res(TPASS, "Offset (%s) is correct %llims",
		        tst_clock_name(tc->clk_id), diff);
	}

	diff = tv->diff(tv->pthen, tv->now);

	if (diff/1000) {
		tst_res(TFAIL, "Wrong offset (%s) read %llims",
		        tst_clock_name(tc->clk_id), diff);
	} else {
		tst_res(TPASS, "Offset (%s) is correct %llims",
		        tst_clock_name(tc->clk_id), diff);
	}
}

static void verify_ns_clock(unsigned int n)
{
	struct test_variants *tv = &variants[tst_variant];
	struct tcase *tc = &tcases[n];

	SAFE_UNSHARE(CLONE_NEWTIME);

	SAFE_FILE_PRINTF("/proc/self/timens_offsets", "%d %d 0",
	                 tc->clk_off, tc->off);

	if (tv->func(tc->clk_id, tv->now)) {
		tst_brk(TBROK | TERRNO, "%d clock_gettime(%s) failed",
			__LINE__, tst_clock_name(tc->clk_id));
	}

	if (!SAFE_FORK())
		child(tv, tc);
}

static void setup(void)
{
	tst_res(TINFO, "Testing variant: %s", variants[tst_variant].desc);

	parent_ns = SAFE_OPEN("/proc/self/ns/time_for_children", O_RDONLY);
}

static void cleanup(void)
{
	SAFE_CLOSE(parent_ns);
}

static struct tst_test test = {
	.setup = setup,
	.cleanup = cleanup,
	.tcnt = ARRAY_SIZE(tcases),
	.test = verify_ns_clock,
	.test_variants = ARRAY_SIZE(variants),
	.needs_root = 1,
	.forks_child = 1,
	.needs_kconfigs = (const char *[]) {
		"CONFIG_TIME_NS=y",
		NULL
	}
};
