// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) Crackerjack Project., 2007
 * Ported from Crackerjack to LTP by Masatake YAMATO <yamato@redhat.com>
 * Copyright (c) 2011 Cyril Hrubis <chrubis@suse.cz>
 * Copyright (c) 2021 Xie Ziyao <xieziyao@huawei.com>
 */

/*\
 * [Description]
 *
 * Test io_getevents invoked via libaio with invalid ctx and expects it to
 * return -EINVAL.
 */

#include "config.h"
#include "tst_test.h"

#ifdef HAVE_LIBAIO
#include <libaio.h>

#define EXP_RET (-EINVAL)

static void run(void)
{
	io_context_t ctx;

	memset(&ctx, 0, sizeof(ctx));
	TEST(io_getevents(ctx, 0, 0, NULL, NULL));

	if (TST_RET == 0)
		tst_res(TFAIL, "call succeeded unexpectedly");
	else if (TST_RET == EXP_RET)
		tst_res(TPASS, "io_cancel(ctx, NULL, NULL) returns %ld : %s",
			TST_RET, strerror(-TST_RET));
	else
		tst_res(TFAIL, "io_cancel(ctx, NULL, NULL) returns %ld : %s, expected %d : %s",
			TST_RET, strerror(-TST_RET), EXP_RET, strerror(-EXP_RET));
}

static struct tst_test test = {
	.needs_kconfigs = (const char *[]) {
		"CONFIG_AIO=y",
		NULL
	},
	.test_all = run,
};

#else
TST_TEST_TCONF("test requires libaio and it's development packages");
#endif
