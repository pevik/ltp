// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines  Corp., 2001
 * Copyright (c) 2024 SUSE LLC <mdoucha@suse.cz>
 */

/*\
 * [Description]
 *
 * Test to check the error conditions in waitpid() syscall.
 */

#include <sys/signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "tst_test.h"

#define TCFMT "waipid(%d, NULL, 0x%x)"
#define TCFMTARGS(tc) (tc)->pid, (tc)->flags

static struct testcase {
	pid_t pid;
	int flags;
	int err;
} testcase_list[] = {
	{-1, 0, ECHILD},	/* Wait for any child when none exist */
	{1, 0, ECHILD},		/* Wait for non-child process */
	{-1, -1, EINVAL},	/* Invalid flags */
	{INT_MIN, 0, ESRCH},	/* Wait for invalid process group */
};

void run(unsigned int n)
{
	const struct testcase *tc = testcase_list + n;

	TEST(waitpid(tc->pid, NULL, tc->flags));

	if (TST_RET == -1 && TST_ERR == tc->err) {
		tst_res(TPASS | TTERRNO, TCFMT " failed as expected",
			TCFMTARGS(tc));
		return;
	}

	if (TST_RET == -1) {
		tst_res(TFAIL | TTERRNO, TCFMT ": expected error %s, got",
			TCFMTARGS(tc), tst_strerrno(tc->err));
		return;
	}

	if (TST_RET < 0) {
		tst_res(TFAIL | TTERRNO, TCFMT ": invalid return value %ld",
			TCFMTARGS(tc), TST_RET);
		return;
	}

	tst_res(TFAIL, TCFMT " returned unexpected PID %ld", TCFMTARGS(tc),
		TST_RET);
}

static struct tst_test test = {
	.test = run,
	.tcnt = ARRAY_SIZE(testcase_list)
};
