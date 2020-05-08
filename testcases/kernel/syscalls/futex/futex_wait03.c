// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2015 Cyril Hrubis <chrubis@suse.cz>
 *
 * Block on a futex and wait for wakeup.
 *
 * This tests uses private mutexes with threads.
 */

#include <errno.h>
#include <pthread.h>

#include "futextest.h"
#include "futex_utils.h"
#include "lapi/abisize.h"

static futex_t futex = FUTEX_INITIALIZER;

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

static void *threaded(void *arg LTP_ATTRIBUTE_UNUSED)
{
	struct test_variants *tv = &variants[tst_variant];
	long ret;

	process_state_wait2(getpid(), 'S');

	ret = futex_wake(tv->fntype, &futex, 1, FUTEX_PRIVATE_FLAG);

	return (void*)ret;
}

static void run(void)
{
	struct test_variants *tv = &variants[tst_variant];
	long ret, res;
	pthread_t t;

	res = pthread_create(&t, NULL, threaded, NULL);
	if (res) {
		tst_res(TFAIL | TTERRNO, "pthread_create() failed");
		return;
	}

	res = futex_wait(tv->fntype, &futex, futex, NULL, FUTEX_PRIVATE_FLAG);
	if (res) {
		tst_res(TFAIL | TTERRNO, "futex_wait() failed");
		pthread_join(t, NULL);
		return;
	}

	res = pthread_join(t, (void*)&ret);
	if (res) {
		tst_res(TFAIL | TTERRNO, "pthread_join() failed");
		return;
	}

	if (ret != 1)
		tst_res(TFAIL, "futex_wake() returned %li", ret);
	else
		tst_res(TPASS, "futex_wait() woken up");
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
