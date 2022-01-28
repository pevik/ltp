// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) Crackerjack Project., 2007
 * Copyright (c) Manas Kumar Nayak maknayak@in.ibm.com>
 * Copyright (C) 2021 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * This test is checking if waitid() syscall filters children which exited from
 * the same group ID.
 */

#include <sys/wait.h>
#include "tst_test.h"

static void run(void)
{
	siginfo_t infop;
	pid_t pid_group;

	/* dummy fork to spawn child in the same group ID */
	if (!SAFE_FORK())
		return;

	pid_group = getpgid(0);

	tst_res(TINFO, "filter child by group ID and WEXITED");

	memset(&infop, 0, sizeof(infop));
	TEST(waitid(P_PGID, pid_group, &infop, WEXITED));

	tst_res(TINFO, "si_pid = %d ; si_code = %d ; si_status = %d",
		infop.si_pid, infop.si_code, infop.si_status);

	if (!TST_RET && !TST_ERR)
		tst_res(TPASS, "waitid returned SUCCESS");
	else
		tst_res(TFAIL, "ret=%ld errno=%s", TST_RET,
			tst_strerrno(TST_ERR));
}

static struct tst_test test = {
	.test_all = run,
	.forks_child = 1,
};
