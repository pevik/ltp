// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2020 Viresh Kumar <viresh.kumar@linaro.org>
 *
 * Description:
 * Basic pidfd_open() test, fetches the PID of the current process and tries to
 * get its file descriptor.
 */

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include "tst_test.h"
#include "lapi/pidfd_open.h"

static void run(void)
{
	int pidfd = 0, flag = 0;

	pidfd = pidfd_open(getpid(), 0);
	if (pidfd == -1)
		tst_brk(TFAIL | TERRNO, "pidfd_open(getpid(), 0) failed");

	flag = SAFE_FCNTL(pidfd, F_GETFD);

	SAFE_CLOSE(pidfd);

	if (!(flag & FD_CLOEXEC))
		tst_brk(TFAIL, "pidfd_open(getpid(), 0) didn't set close-on-exec flag");

	tst_res(TPASS, "pidfd_open(getpid(), 0) passed");
}

static struct tst_test test = {
	.test_all = run,
};
