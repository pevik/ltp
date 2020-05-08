// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2015 Cyril Hrubis <chrubis@suse.cz>
 *
 * Block several threads on a private mutex, then wake them up.
 */

#include <errno.h>
#include <pthread.h>
#include <sys/types.h>

#include "futextest.h"
#include "futex_utils.h"
#include "lapi/abisize.h"

static futex_t futex = FUTEX_INITIALIZER;

static volatile int threads_flags[55];

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

static int threads_awake(void)
{
	int ret = 0;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(threads_flags); i++) {
		if (threads_flags[i])
			ret++;
	}

	return ret;
}

static void clear_threads_awake(void)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(threads_flags); i++)
		threads_flags[i] = 0;
}

static void *threaded(void *arg)
{
	struct test_variants *tv = &variants[tst_variant];
	long i = (long)arg;

	futex_wait(tv->fntype, &futex, futex, NULL, FUTEX_PRIVATE_FLAG);

	threads_flags[i] = 1;

	return NULL;
}

static void do_child(void)
{
	struct test_variants *tv = &variants[tst_variant];
	int res, i, j, awake;
	pthread_t t[55];

	for (i = 0; i < (int)ARRAY_SIZE(t); i++) {
		res = pthread_create(&t[i], NULL, threaded, (void*)((long)i));
		if (res) {
			tst_res(TFAIL | TTERRNO, "pthread_create() failed");
			return;
		}
	}

	while (wait_for_threads(ARRAY_SIZE(t)))
		usleep(100);

	for (i = 1; i <= 10; i++) {
		clear_threads_awake();
		res = futex_wake(tv->fntype, &futex, i, FUTEX_PRIVATE_FLAG);
		if (i != res) {
			tst_res(TFAIL | TTERRNO,
			         "futex_wake() woken up %i threads, expected %i",
			         res, i);
		}

		for (j = 0; j < 100000; j++) {
			awake = threads_awake();
			if (awake == i)
				break;

			usleep(100);
		}

		if (awake == i) {
			tst_res(TPASS, "futex_wake() woken up %i threads", i);
		} else {
			tst_res(TFAIL | TTERRNO, "Woken up %i threads, expected %i",
				awake, i);
		}
	}

	res = futex_wake(tv->fntype, &futex, 1, FUTEX_PRIVATE_FLAG);

	if (res) {
		tst_res(TFAIL | TTERRNO, "futex_wake() woken up %i, none were waiting",
			res);
	} else {
		tst_res(TPASS, "futex_wake() woken up 0 threads");
	}

	for (i = 0; i < (int)ARRAY_SIZE(t); i++)
		pthread_join(t[i], NULL);

	exit(0);
}

/*
 * We do the real test in a child because with the test -i parameter the loop
 * that checks that all threads are sleeping may fail with ENOENT. That is
 * because some of the threads from previous run may still be there.
 *
 * Which is because the userspace part of pthread_join() sleeps in a futex on a
 * pthread tid which is woken up at the end of the exit_mm(tsk) which is before
 * the process is removed from the parent thread_group list. So there is a
 * small race window where the readdir() returns the process tid as a directory
 * under /proc/$PID/tasks/, but the subsequent open() fails with ENOENT because
 * the thread was removed meanwhile.
 */
static void run(void)
{
	int status, pid;

	pid = SAFE_FORK();

	switch (pid) {
	case 0:
		do_child();
	default:
		SAFE_WAITPID(pid, &status, 0);
	}
}

static void setup(void)
{
	tst_res(TINFO, "Testing variant: %s", variants[tst_variant].desc);
}

static struct tst_test test = {
	.setup = setup,
	.test_all = run,
	.test_variants = ARRAY_SIZE(variants),
	.forks_child = 1,
};
