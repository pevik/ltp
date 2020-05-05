/*
 * Copyright (C) 2015 Cyril Hrubis <chrubis@suse.cz>
 *
 * Licensed under the GNU GPLv2 or later.
 * This program is free software;  you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY;  without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program;  if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
 /*
  * futex_wake() returns 0 (0 woken up processes) when no processes wait on the mutex.
  */

#include <errno.h>
#include <limits.h>

#include "futextest.h"
#include "lapi/abisize.h"

struct testcase {
	futex_t *f_addr;
	int nr_wake;
	int opflags;
};

static futex_t futex = FUTEX_INITIALIZER;

static struct testcase testcases[] = {
	/* nr_wake = 0 is noop */
	{&futex, 0, 0},
	{&futex, 0, FUTEX_PRIVATE_FLAG},
	{&futex, 1, 0},
	{&futex, 1, FUTEX_PRIVATE_FLAG},
	{&futex, INT_MAX, 0},
	{&futex, INT_MAX, FUTEX_PRIVATE_FLAG},
};

static struct test_variants {
	enum futex_fn_type fntype;
	char *desc;
} variants[] = {
#if defined(TST_ABI32)
	{ .fntype = FUTEX_FN_FUTEX, .desc = "syscall with kernel spec32"},
#endif

#if defined(TST_ABI64)
	{ .fntype = FUTEX_FN_FUTEX, .desc = "syscall with kernel spec64"},
#endif

#if (__NR_futex_time64 != __LTP__NR_INVALID_SYSCALL)
	{ .fntype = FUTEX_FN_FUTEX64, .desc = "syscall time64 with kernel spec64"},
#endif
};

static void run(unsigned int n)
{
	struct test_variants *tv = &variants[tst_variant];
	struct testcase *tc = &testcases[n];
	int res;

	res = futex_wake(tv->fntype, tc->f_addr, tc->nr_wake, tc->opflags);
	if (res != 0) {
		tst_res(TFAIL | TTERRNO, "futex_wake() failed");
		return;
	}

	tst_res(TPASS, "futex_wake() passed");
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
