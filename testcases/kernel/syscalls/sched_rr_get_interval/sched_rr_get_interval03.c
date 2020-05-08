// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) Wipro Technologies Ltd, 2002.  All Rights Reserved.
 *
 *    TEST IDENTIFIER	: sched_rr_get_interval03
 *
 *    EXECUTED BY	: root / superuser
 *
 *    TEST TITLE	: Tests for error conditions
 *
 *    TEST CASE TOTAL	: 3
 *
 *    AUTHOR		: Saji Kumar.V.R <saji.kumar@wipro.com>
 *
 *    SIGNALS
 *	Uses SIGUSR1 to pause before test if option set.
 *	(See the parse_opts(3) man page).
 *
 *    DESCRIPTION
 *	Verify that
 *	1) sched_rr_get_interval() fails with errno set to EINVAL for an
 *	   invalid pid
 *	2) sched_rr_get_interval() fails with errno set to ESRCH if the
 *	   process with specified pid does not exists
 *	2) sched_rr_get_interval() fails with errno set to EFAULT if the
 *	   address specified as &tp is invalid
 *
 *	Setup:
 *	  Setup signal handling.
 *	  Set expected errors for logging.
 *	  Pause for SIGUSR1 if option specified.
 *	  Change scheduling policy to SCHED_RR
 *
 *	Test:
 *	 Loop if the proper options are given.
 *	  Execute system call
 *	  if call fails with expected errno,
 *		Test passed.
 *	  Otherwise
 *		Test failed
 *
 *	Cleanup:
 *	  Print errno log and/or timing stats if options given
 *
 * USAGE:  <for command-line>
 * sched_rr_get_interval03 [-c n] [-e] [-i n] [-I x] [-P x] [-t] [-h] [-f] [-p]
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

static pid_t unused_pid;
static pid_t inval_pid = -1;
static pid_t zero_pid;

struct tst_ts tp;
static void *bad_addr;

struct test_cases_t {
	pid_t *pid;
	struct tst_ts *tp;
	int exp_errno;
} test_cases[] = {
	{ &inval_pid, &tp, EINVAL},
	{ &unused_pid, &tp, ESRCH},
#ifndef UCLINUX
	/* Skip since uClinux does not implement memory protection */
	{ &zero_pid, NULL, EFAULT}
#endif
};

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

	bad_addr = tst_get_bad_addr(NULL);
	tp.type = tv->type;

	/* Change scheduling policy to SCHED_RR */
	if ((sched_setscheduler(0, SCHED_RR, &p)) == -1)
		tst_res(TFAIL | TTERRNO, "sched_setscheduler() failed");

	unused_pid = tst_get_unused_pid();
}

static void run(unsigned int i)
{
	struct test_variants *tv = &variants[tst_variant];
	struct test_cases_t *tc = &test_cases[i];
	struct timerspec *ts;

	if (tc->exp_errno == EFAULT)
		ts = bad_addr;
	else
		ts = tst_ts_get(tc->tp);

	/*
	 * Call sched_rr_get_interval(2)
	 */
	TEST(tv->func(*tc->pid, ts));

	if (TST_RET != -1) {
		tst_res(TFAIL, "sched_rr_get_interval() passed unexcpectedly");
		return;
	}

	if (tc->exp_errno == TST_ERR)
		tst_res(TPASS | TTERRNO, "sched_rr_get_interval() failed as excpected");
	else
		tst_res(TFAIL | TTERRNO, "sched_rr_get_interval() failed unexcpectedly: %s",
			tst_strerrno(tc->exp_errno));
}

static struct tst_test test = {
	.test = run,
	.tcnt = ARRAY_SIZE(test_cases),
	.test_variants = ARRAY_SIZE(variants),
	.setup = setup,
	.needs_root = 1,
};
