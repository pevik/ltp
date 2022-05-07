// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright(c) 2022 Huawei Technologies Co., Ltd
 * Author: Li Mengfei <limengfei4@huawei.com>
 *         Zhao Gongyi <zhaogongyi@huawei.com>
 */

/*\
 * [Description]
 *
 * 1. Create a high nice thread and a low nice thread, the main
 *    thread wake them at the same time
 * 2. Both threads run on the same CPU
 * 3. Verify that the low nice thread execute more cycles than
 *    the high nice thread
 */

#define _GNU_SOURCE
#include <pthread.h>
#include "tst_test.h"
#include "tst_safe_pthread.h"

#define LIMIT_TIME 3
#define RUN_TIMES 1000000

static pthread_barrier_t barrier;
static long long time_nice_low, time_nice_high;
static int some_cpu;
static cpu_set_t *set;

static void set_nice(int nice_inc)
{
	int orig_nice;

	orig_nice = SAFE_GETPRIORITY(PRIO_PROCESS, 0);
	TEST(nice(nice_inc));

	if (TST_RET != (orig_nice + nice_inc)) {
		tst_brk(TFAIL | TTERRNO, "nice(%d) returned %li, expected %i",
				nice_inc, TST_RET, orig_nice + nice_inc);
	}

	if (TST_ERR)
		tst_brk(TFAIL | TTERRNO, "nice(%d) failed", nice_inc);

}

static void* nice_low_thread(void* arg)
{
	int number = 0;
	int ret = 0;

	set_nice(*(int*)arg);
	ret = pthread_barrier_wait(&barrier);
	if (ret != 0 && ret != PTHREAD_BARRIER_SERIAL_THREAD) {
		tst_brk(TBROK, "pthread_barrier_wait() returned %s",
				tst_strerrno(-ret));
	}

	while (1) {
		for (int i = 0; i < RUN_TIMES; ++i)
			number++;

		pthread_testcancel();
		time_nice_low++;
	}
	return NULL;
}

static void* nice_high_thread(void* arg)
{
	int number = 0;
	int ret = 0;

	set_nice(*(int*)arg);
	ret = pthread_barrier_wait(&barrier);
	if (ret != 0 && ret != PTHREAD_BARRIER_SERIAL_THREAD) {
		tst_brk(TBROK, "pthread_barrier_wait() returned %s",
				tst_strerrno(-ret));
	}

	while (1) {
		for (int i = 0; i < RUN_TIMES; ++i)
			number++;

		pthread_testcancel();
		time_nice_high++;
	}
	return NULL;
}

static void setup(void)
{
	size_t size;
	int index;
	int nrcpus = 1024;

	set = CPU_ALLOC(nrcpus);
	if (!set)
		tst_brk(TBROK | TERRNO, "CPU_ALLOC()");

	size = CPU_ALLOC_SIZE(nrcpus);
	CPU_ZERO_S(size, set);
	if (sched_getaffinity(0, size, set) < 0) {
		tst_brk(TBROK | TERRNO, "sched_getaffinity()");
	}

	for (index = 0; index < (int)size * 8; index++)
		if (CPU_ISSET_S(index, size, set))
			some_cpu = index;

	CPU_ZERO_S(size, set);
	CPU_SET_S(some_cpu, size, set);
	if (sched_setaffinity(0, size, set) < 0) {
		tst_brk(TBROK | TERRNO, "sched_setaffinity()");
	}
}

static void cleanup(void)
{
	if (set)
		CPU_FREE(set);
}

static void verify_nice(void)
{
	int ret;
	int nice_inc_high = -1;
	int nice_inc_low = -2;
	pthread_t nice_low, nice_high;

	ret = pthread_barrier_init(&barrier, NULL, 3);
	if (ret != 0) {
		tst_brk(TBROK, "pthread_barrier_init() returned %s",
			tst_strerrno(-ret));
	}

	SAFE_PTHREAD_CREATE(&nice_high, NULL, nice_high_thread, (void*)&nice_inc_high);
	SAFE_PTHREAD_CREATE(&nice_low, NULL, nice_low_thread, (void*)&nice_inc_low);

	pthread_barrier_wait(&barrier);
	if (ret != 0 && ret != PTHREAD_BARRIER_SERIAL_THREAD)
		tst_brk(TBROK | TERRNO, "pthread_barrier_wait() failed");

	sleep(LIMIT_TIME);

	ret = pthread_cancel(nice_high);
	if(ret) {
		tst_brk(TBROK, "pthread_cancel() returned %s",
			tst_strerrno(-ret));
	}

	ret = pthread_cancel(nice_low);
	if(ret) {
		tst_brk(TBROK, "pthread_cancel() returned %s",
			tst_strerrno(-ret));
	}

	ret = pthread_barrier_destroy(&barrier);
	if (ret) {
		tst_brk(TBROK, "pthread_barrier_destroy() returned %s",
			tst_strerrno(-ret));
	}

	SAFE_PTHREAD_JOIN(nice_high, NULL);
	SAFE_PTHREAD_JOIN(nice_low, NULL);

	if (time_nice_low > time_nice_high) {
		tst_res(TPASS, "time_nice_low = %lld time_nice_high = %lld",
			time_nice_low, time_nice_high);
	} else {
		tst_brk(TFAIL | TTERRNO, "Test failed :"
			"time_nice_low = %lld time_nice_high = %lld",
			time_nice_low, time_nice_high);
	}
}

static struct tst_test test = {
	.setup = setup,
	.cleanup = cleanup,
	.test_all = verify_nice,
	.needs_root = 1,
};
