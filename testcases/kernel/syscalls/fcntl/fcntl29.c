// SPDX-License-Identifier: GPL-2.0-or-later

/*
 * Copyright (c) 2014 Fujitsu Ltd.
 * Author: Xiaoguang Wang <wangxg.fnst@cn.fujitsu.com>
 * Copyright (C) 2024 SUSE LLC Andrea Manzini <andrea.manzini@suse.com>
 */

/*\
 * [Description]
 * Basic test for fcntl(2) using F_DUPFD_CLOEXEC argument.
 */

#include "lapi/fcntl.h"

#include <tst_test.h>

static int test_fd;

static void setup(void)
{
	test_fd = SAFE_CREAT("testfile", 0644);
}

static void cleanup(void)
{
	if (test_fd > 0)
		SAFE_CLOSE(test_fd);
}

static void run(void)
{
	TST_EXP_FD(fcntl(test_fd, F_DUPFD_CLOEXEC, 0));
	int dup_fd = TST_RET;

	TST_EXP_POSITIVE(fcntl(dup_fd, F_GETFD));
	if (TST_RET & FD_CLOEXEC)
		tst_res(TPASS, "fcntl test F_DUPFD_CLOEXEC success");
	else
		tst_res(TFAIL, "fcntl test F_DUPFD_CLOEXEC failed");
	SAFE_CLOSE(dup_fd);
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.cleanup = cleanup,
	.needs_tmpdir = 1,
};
