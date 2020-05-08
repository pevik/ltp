// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2015 Cyril Hrubis <chrubis@suse.cz>
 *
 * Based on futextest (futext_wait_uninitialized_heap.c)
 * written by KOSAKI Motohiro <kosaki.motohiro@jp.fujitsu.com>
 *
 * Wait on uninitialized heap. It shold be zero and FUTEX_WAIT should return
 * immediately. This test tests zero page handling in futex code.
 */

#include <errno.h>

#include "futextest.h"
#include "lapi/abisize.h"

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

static void run(void)
{
	struct test_variants *tv = &variants[tst_variant];
	static struct tst_ts to;
	size_t pagesize = getpagesize();
	void *buf;
	int res;

	buf = SAFE_MMAP(NULL, pagesize, PROT_READ|PROT_WRITE,
			MAP_PRIVATE|MAP_ANONYMOUS, 0, 0);

	to.type = tv->tstype;
	tst_ts_set_sec(&to, 0);
	tst_ts_set_nsec(&to, 10000);

	res = futex_wait(tv->fntype, buf, 1, &to, 0);
	if (res == -1 && errno == EWOULDBLOCK)
		tst_res(TPASS | TERRNO, "futex_wait() returned %i", res);
	else
		tst_res(TFAIL | TERRNO, "futex_wait() returned %i", res);

	SAFE_MUNMAP(buf, pagesize);
}

static void setup(void)
{
	tst_res(TINFO, "Testing variant: %s", variants[tst_variant].desc);
}

static struct tst_test test = {
	.setup = setup,
	.test_all = run,
	.test_variants = ARRAY_SIZE(variants),
};
