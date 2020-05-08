// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright (c) Crackerjack Project., 2007 */

#include <time.h>
#include <signal.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <errno.h>

#include "tst_timer.h"
#include "lapi/abisize.h"

static struct test_variants {
	int (*func)(kernel_timer_t timer, void *its);
	enum tst_ts_type type;
	char *desc;
} variants[] = {
#if defined(TST_ABI32)
	{ .func = sys_timer_gettime, .type = TST_LIBC_TIMESPEC, .desc = "syscall with libc spec"},
#endif

#if defined(TST_ABI64)
	{ .func = sys_timer_gettime, .type = TST_KERN_TIMESPEC, .desc = "syscall with kernel spec64"},
#endif

#if (__NR_timer_gettime64 != __LTP__NR_INVALID_SYSCALL)
	{ .func = sys_timer_gettime64, .type = TST_KERN_TIMESPEC, .desc = "syscall time64 with kernel spec64"},
#endif
};

static kernel_timer_t timer;

static void setup(void)
{
	struct sigevent ev;

	tst_res(TINFO, "Testing variant: %s", variants[tst_variant].desc);

	ev.sigev_value = (union sigval) 0;
	ev.sigev_signo = SIGALRM;
	ev.sigev_notify = SIGEV_SIGNAL;

	TEST(tst_syscall(__NR_timer_create, CLOCK_REALTIME, &ev, &timer));

	if (TST_RET) {
		tst_res(TFAIL | TTERRNO, "timer_create() failed");
		return;
	}
}

static void verify(void)
{
	struct test_variants *tv = &variants[tst_variant];
	struct tst_its spec = {.type = tv->type, };

	TEST(tv->func(timer, tst_its_get(&spec)));
	if (TST_RET == 0) {
		tst_res(TPASS, "timer_gettime() Passed");
	} else {
		tst_res(TFAIL | TTERRNO, "timer_gettime() Failed");
	}

	TEST(tv->func((kernel_timer_t)-1, tst_its_get(&spec)));
	if (TST_RET == -1 && TST_ERR == EINVAL)
		tst_res(TPASS, "timer_gettime(-1) Failed: EINVAL");
	else
		tst_res(TFAIL | TTERRNO, "timer_gettime(-1) = %li", TST_RET);

	TEST(tv->func(timer, NULL));
	if (TST_RET == -1 && TST_ERR == EFAULT)
		tst_res(TPASS, "timer_gettime(NULL) Failed: EFAULT");
	else
		tst_res(TFAIL | TTERRNO, "timer_gettime(-1) = %li", TST_RET);
}

static struct tst_test test = {
	.test_all = verify,
	.test_variants = ARRAY_SIZE(variants),
	.setup = setup,
	.needs_tmpdir = 1,
};
