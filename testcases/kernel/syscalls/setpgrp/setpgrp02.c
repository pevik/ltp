// SPDX-License-Identifier: GPL-2.0-or-later

/*
 * Copyright (c) International Business Machines  Corp., 2001
 *	07/2001 Ported by Wayne Boyer
 * Copyright (C) 2024 SUSE LLC Andrea Manzini <andrea.manzini@suse.com>
 */

/*\
 * [Description]
 * Testcase to check the basic functionality of the setpgrp(2) syscall.
 */

#include "tst_test.h"

static void verify_setpgrp(void)
{
	/* fork() is needed in order to make sure to have a different PGID */
	int pid = SAFE_FORK();

	if (pid == 0) { // child
		int oldpgrp = getpgrp();

		TST_EXP_PASS(setpgrp());
		if (getpgrp() == oldpgrp)
			tst_res(TFAIL, "setpgrp() FAILED to set new group id");
		else
			tst_res(TPASS, "functionality is correct");
	} else { // parent
		int status;

		SAFE_WAIT(&status);
		if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
			tst_res(TFAIL, "child 1: %s", tst_strstatus(status));
	}
}

static struct tst_test test = {
	.test_all = verify_setpgrp,
	.forks_child = 1
};
