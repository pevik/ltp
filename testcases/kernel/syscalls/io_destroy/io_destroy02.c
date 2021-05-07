// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) Crackerjack Project., 2007
 * Ported from Crackerjack to LTP by Masatake YAMATO <yamato@redhat.com>
 * Copyright (c) 2011 Cyril Hrubis <chrubis@suse.cz>
 * Copyright (c) 2017 Xiao Yang <yangx.jy@cn.fujitsu.com>
 * Copyright (c) 2021 Xie Ziyao <xieziyao@huawei.com>
 */

/*\
 * [Description]
 *
 * Test io_destroy invoked via syscall(2) with an invalid ctx and expects
 * it to return EINVAL.
 */

#include <linux/aio_abi.h>

#include "config.h"
#include "tst_test.h"
#include "lapi/syscalls.h"

static void verify_io_destroy(void)
{
	aio_context_t ctx;
	memset(&ctx, 0xff, sizeof(ctx));
	TST_EXP_FAIL(tst_syscall(__NR_io_destroy, ctx), EINVAL);
}

static struct tst_test test = {
	.test_all = verify_io_destroy,
};
