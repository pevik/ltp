// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Verify that lsm_set_self_attr syscall is raising errors when invalid data is
 * provided.
 */

#include "lsm_common.h"

static struct lsm_ctx *ctx;
static struct lsm_ctx *ctx_orig;
static struct lsm_ctx *ctx_null;
static uint32_t ctx_size;
static uint32_t ctx_size_small;
static uint32_t ctx_size_big;
static uint32_t page_size;

static struct tcase {
	uint32_t attr;
	struct lsm_ctx **ctx;
	uint32_t *size;
	uint32_t flags;
	int exp_errno;
	char *msg;
} tcases[] = {
	{
		.attr = LSM_ATTR_CURRENT,
		.ctx = &ctx_null,
		.size = &ctx_size,
		.exp_errno = EFAULT,
		.msg = "ctx is NULL",
	},
	{
		.attr = LSM_ATTR_CURRENT,
		.ctx = &ctx,
		.size = &ctx_size_small,
		.exp_errno = EINVAL,
		.msg = "size is too small",
	},
	{
		.attr = LSM_ATTR_CURRENT,
		.ctx = &ctx,
		.size = &ctx_size_big,
		.exp_errno = E2BIG,
		.msg = "size is too big",
	},
	{
		.attr = LSM_ATTR_CURRENT,
		.ctx = &ctx,
		.size = &ctx_size,
		.flags = 1,
		.exp_errno = EINVAL,
		.msg = "flags must be zero",
	},
	{
		.attr = LSM_ATTR_CURRENT | LSM_ATTR_EXEC,
		.ctx = &ctx,
		.size = &ctx_size,
		.exp_errno = EINVAL,
		.msg = "attr is overset",
	}
};

static void run(unsigned int n)
{
	struct tcase *tc = &tcases[n];

	/* just in case lsm_set_self_attr() pass , we won't change
	 * LSM configuration for the following process
	 */
	memcpy(ctx, ctx_orig, sizeof(struct lsm_ctx));

	ctx_size = page_size;
	ctx_size_small = 1;
	ctx_size_big = ctx_size + 1;

	TST_EXP_FAIL(lsm_set_self_attr(tc->attr, *tc->ctx, *tc->size, tc->flags),
	      tc->exp_errno,
	      "%s", tc->msg);
}

static void setup(void)
{
	int ret;
	uint32_t size;

	verify_supported_attr_current();

	page_size = SAFE_SYSCONF(_SC_PAGESIZE);
	size = page_size;

	ret = lsm_get_self_attr(LSM_ATTR_CURRENT, ctx_orig, &size, 0);
	if (ret < 0)
		tst_brk(TBROK, "Can't read LSM current attribute");
}

static struct tst_test test = {
	.test = run,
	.setup = setup,
	.tcnt = ARRAY_SIZE(tcases),
	.min_kver = "6.8",
	.bufs = (struct tst_buffers[]) {
		{&ctx, .size = sizeof(struct lsm_ctx)},
		{&ctx_orig, .size = sizeof(struct lsm_ctx)},
		{}
	},
};
