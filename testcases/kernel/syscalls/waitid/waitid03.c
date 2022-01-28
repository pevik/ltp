// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) Crackerjack Project., 2007
 * Copyright (c) Manas Kumar Nayak maknayak@in.ibm.com>
 * Copyright (C) 2021 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * This test is checking if waitid() syscall returns ECHILD when the calling
 * process has no existing unwaited-for child processes.
 */

#include <sys/wait.h>
#include "tst_test.h"

static void run(void)
{
	siginfo_t infop;

	memset(&infop, 0, sizeof(infop));
	TEST(waitid(P_ALL, 0, &infop, WNOHANG | WEXITED));

	tst_res(TINFO, "si_pid = %d ; si_code = %d ; si_status = %d",
		infop.si_pid, infop.si_code, infop.si_status);

	if (TST_RET == -1 && TST_ERR == ECHILD)
		tst_res(TPASS, "waitid returned ECHILD");
	else
		tst_res(TFAIL, "ret=%ld errno=%s", TST_RET,
			tst_strerrno(TST_ERR));
}

static struct tst_test test = {
	.test_all = run,
};
