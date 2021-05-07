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
 * Test io_setup invoked via syscall(2):
 * - io_setup fails and returns -EFAULT if ctxp is NULL;
 * - io_setup fails and returns -EINVAL if ctxp is not initialized to 0;
 * - io_setup fails and returns -EINVAL if nr_events is -1;
 * - io_setup fails and returns -EAGAIN if nr_events exceeds the limit
 *   of available events;
 * - io_setup succeeds if both nr_events and ctxp are valid.
 */

#include <linux/aio_abi.h>

#include "config.h"
#include "tst_test.h"
#include "lapi/syscalls.h"

static void verify_io_setup(void)
{
	aio_context_t ctx;
	TST_EXP_FAIL(tst_syscall(__NR_io_setup, 1, NULL), EFAULT);

	memset(&ctx, 1, sizeof(ctx));
	TST_EXP_FAIL(tst_syscall(__NR_io_setup, 1, &ctx), EINVAL);
	memset(&ctx, 0, sizeof(ctx));
	TST_EXP_FAIL(tst_syscall(__NR_io_setup, -1, &ctx), EINVAL);

	unsigned aio_max = 0;
	if (!access("/proc/sys/fs/aio-max-nr", F_OK)) {
		SAFE_FILE_SCANF("/proc/sys/fs/aio-max-nr", "%u", &aio_max);
		TST_EXP_FAIL(tst_syscall(__NR_io_setup, aio_max + 1, &ctx), EAGAIN);
	} else {
		tst_res(TCONF, "the aio-max-nr file did not exist");
	}

	TST_EXP_PASS(tst_syscall(__NR_io_setup, 1, &ctx));
	TST_EXP_PASS(tst_syscall(__NR_io_destroy, ctx));
}

static struct tst_test test = {
	.test_all = verify_io_setup,
};
