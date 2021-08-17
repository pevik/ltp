// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
 * Author: Xie Ziyao <xieziyao@huawei.com>
 */

/*\
 * [Description]
 *
 * Verify that epoll_create return a nonnegative file descriptor on success.
 *
 * The size argument informed the kernel of the number of file descriptors
 * that the caller expected to add to the epoll instance, but it is no longer
 * required.
 */

#include <sys/epoll.h>

#include "tst_test.h"
#include "lapi/epoll.h"
#include "lapi/syscalls.h"

static int tc[] = {1, INT_MAX};

static void run(unsigned int n)
{
	int fd;

	fd = tst_syscall(__NR_epoll_create, tc[n]);
	if (fd < 0)
		tst_brk(TFAIL | TERRNO, "epoll_create(%d) failed", tc[n]);
	tst_res(TPASS, "epoll_create(%d)", tc[n]);

	SAFE_CLOSE(fd);
}

static struct tst_test test = {
	.tcnt = ARRAY_SIZE(tc),
	.test = run,
};
