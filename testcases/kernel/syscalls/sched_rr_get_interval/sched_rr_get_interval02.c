// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) Wipro Technologies Ltd, 2002.  All Rights Reserved.
 *
 *    TEST IDENTIFIER	: sched_rr_get_interval02
 *
 *    EXECUTED BY	: root / superuser
 *
 *    TEST TITLE	: Functionality test
 *
 *    TEST CASE TOTAL	: 1
 *
 *    AUTHOR		: Saji Kumar.V.R <saji.kumar@wipro.com>
 *
 *    SIGNALS
 *	Uses SIGUSR1 to pause before test if option set.
 *	(See the parse_opts(3) man page).
 *
 *    DESCRIPTION
 *	Verify that for a process with scheduling policy SCHED_FIFO,
 *	sched_rr_get_interval() writes zero into timespec structure
 *	for tv_sec & tv_nsec.
 *
 *	Setup:
 *	  Setup signal handling.
 *	  Pause for SIGUSR1 if option specified.
 *	  Change scheduling policy to SCHED_FIFO
 *
 *	Test:
 *	 Loop if the proper options are given.
 *	  Execute system call
 *	  Check (return code == 0) & (got 0 for tv_sec & tv_nsec )
 *		Test passed.
 *	  Otherwise
 *		Test failed
 *
 *	Cleanup:
 *	  Print errno log and/or timing stats if options given
 *
 * USAGE:  <for command-line>
 * sched_rr_get_interval02 [-c n] [-e] [-i n] [-I x] [-P x] [-t] [-h] [-f] [-p]
 *			where,  -c n : Run n copies concurrently.
 *				-e   : Turn on errno logging.
 *				-h   : Show help screen
 *				-f   : Turn off functional testing
 *				-i n : Execute test n times.
 *				-I x : Execute test for x seconds.
 *				-p   : Pause for SIGUSR1 before starting
 *				-P x : Pause for x seconds between iterations.
 *				-t   : Turn on syscall timing.
 *
 */

#include <sched.h>
#include "tst_timer.h"
#include "lapi/abisize.h"

struct tst_ts tp;

static struct test_variants {
	int (*func)(pid_t pid, void *ts);
	enum tst_ts_type type;
	char *desc;
} variants[] = {
#if defined(TST_ABI32)
	{ .func = libc_sched_rr_get_interval, .type = TST_LIBC_TIMESPEC, .desc = "vDSO or syscall with libc spec"},
	{ .func = sys_sched_rr_get_interval, .type = TST_LIBC_TIMESPEC, .desc = "syscall with libc spec"},
	{ .func = sys_sched_rr_get_interval, .type = TST_KERN_OLD_TIMESPEC, .desc = "syscall with kernel spec32"},
#endif

#if defined(TST_ABI64)
	{ .func = sys_sched_rr_get_interval, .type = TST_KERN_TIMESPEC, .desc = "syscall with kernel spec64"},
#endif

#if (__NR_sched_rr_get_interval_time64 != __LTP__NR_INVALID_SYSCALL)
	{ .func = sys_sched_rr_get_interval64, .type = TST_KERN_TIMESPEC, .desc = "syscall time64 with kernel spec64"},
#endif
};

static void setup(void)
{
	struct test_variants *tv = &variants[tst_variant];
	/*
	 * Initialize scheduling parameter structure to use with
	 * sched_setscheduler()
	 */
	struct sched_param p = { 1 };

	tst_res(TINFO, "Testing variant: %s", tv->desc);

	tp.type = tv->type;

	/* Change scheduling policy to SCHED_FIFO */
	if ((sched_setscheduler(0, SCHED_FIFO, &p)) == -1)
		tst_res(TFAIL | TTERRNO, "sched_setscheduler() failed");
}

static void run(void)
{
	struct test_variants *tv = &variants[tst_variant];

	tst_ts_set_sec(&tp, 99);
	tst_ts_set_nsec(&tp, 99);

	/*
	 * Call sched_rr_get_interval(2) with pid=0 so that it will
	 * write into the timespec structure pointed to by tp the
	 * round robin time quantum for the current process.
	 */
	TEST(tv->func(0, tst_ts_get(&tp)));

	if (!TST_RET && tst_ts_valid(&tp) == -1) {
		tst_res(TPASS, "sched_rr_get_interval() passed");
	} else {
		tst_res(TFAIL | TTERRNO, "Test Failed, sched_rr_get_interval() returned %ld, tp.tv_sec = %lld, tp.tv_nsec = %lld",
			TST_RET, tst_ts_get_sec(tp), tst_ts_get_nsec(tp));
	}
}

static struct tst_test test = {
	.test_all = run,
	.test_variants = ARRAY_SIZE(variants),
	.setup = setup,
	.needs_root = 1,
};
