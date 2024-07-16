// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Shell test example.
 */

#include "tst_test.h"

static void run_test(void)
{
	tst_run_shell("shell_test_brk.sh", NULL);
}

static struct tst_test test = {
	.runs_shell = 1,
	.test_all = run_test,
};
