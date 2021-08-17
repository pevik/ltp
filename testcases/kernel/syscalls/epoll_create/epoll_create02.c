// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
 * Author: Xie Ziyao <xieziyao@huawei.com>
 */

/*\
 * [Description]
 *
 * Verify that epoll_create returns -1 and set errno to EINVAL if size is not
 * positive.
 */

#include <sys/epoll.h>

#include "tst_test.h"
#include "lapi/epoll.h"
#include "lapi/syscalls.h"

static struct test_case_t {
	int size;
	int exp_err;
} tc[] = {
	{0, EINVAL},
	{-1, EINVAL}
};

static void run(unsigned int n)
{
	TST_EXP_FAIL(tst_syscall(__NR_epoll_create, tc[n].size),
		     tc[n].exp_err, "create(%d)", tc[n].size);
}

static struct tst_test test = {
	.tcnt = ARRAY_SIZE(tc),
	.test = run,
};
