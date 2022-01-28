// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) Crackerjack Project., 2007
 * Copyright (c) Manas Kumar Nayak maknayak@in.ibm.com>
 * Copyright (C) 2021 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * This test is checking if waitid() syscall filters a child in WNOHANG status.
 */

#include <sys/wait.h>
#include "tst_test.h"

static void run(void)
{
	int status;
	siginfo_t infop;
	pid_t pid_child;

	pid_child = SAFE_FORK();
	if (!pid_child) {
		TST_CHECKPOINT_WAIT(0);
		return;
	}

	tst_res(TINFO, "filter all children by WNOHANG | WEXITED");

	memset(&infop, 0, sizeof(infop));
	TEST(waitid(P_ALL, pid_child, &infop, WNOHANG | WEXITED));

	tst_res(TINFO, "si_pid = %d ; si_code = %d ; si_status = %d",
		infop.si_pid, infop.si_code, infop.si_status);

	if (!TST_RET && !TST_ERR)
		tst_res(TPASS, "waitid returned SUCCESS");
	else
		tst_res(TFAIL, "ret=%ld errno=%s", TST_RET,
			tst_strerrno(TST_ERR));

	TST_CHECKPOINT_WAKE(0);

	SAFE_WAITPID(pid_child, &status, 0);
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
		tst_res(TFAIL, "child returns %d", status);
}

static struct tst_test test = {
	.test_all = run,
	.forks_child = 1,
	.needs_checkpoints = 1,
};
