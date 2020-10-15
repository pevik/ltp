// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2020 Petr Vorel <pvorel@suse.cz>
 * Copyright (c) Linux Test Project, 2009-2020
 * Copyright (c) Crackerjack Project, 2007-2008, Hitachi, Ltd
 *
 * Authors:
 * Takahiro Yasui <takahiro.yasui.mp@hitachi.com>
 * Yumiko Sugita <yumiko.sugita.yf@hitachi.com>
 * Satoshi Fujiwara <sa-fuji@sdl.hitachi.co.jp>
 * Manas Kumar Nayak <maknayak@in.ibm.com> (original port to the legacy API)
 */

/*\
 * [DESCRIPTION]
 *
 * Verify that get_mempolicy() returns a proper return value and errno for various cases.
\*/

#include "config.h"
#include "tst_test.h"

#ifdef HAVE_NUMA_V2
#include <numa.h>
#include <numaif.h>

#include <errno.h>
#include "tst_numa.h"
#include "lapi/syscalls.h"

#define MEM_LENGTH	(4 * 1024 * 1024)
#define PAGES_ALLOCATED 16u

#define POLICY_DESC(x) .policy = x, .desc = "policy: "#x
#define POLICY_DESC_TEXT(x, y) .policy = x, .desc = "policy: "#x", "y
#define POLICY_DESC_FLAGS(x, y) .policy = x, .flags = y, .desc = "policy: "#x", flags: "#y
#define POLICY_DESC_FLAGS_TEXT(x, y, z) .policy = x, .flags = y, .desc = "policy: "#x", flags: "#y", "z

static struct tst_nodemap *node;
static struct bitmask *nodemask, *getnodemask, *empty_nodemask;

struct test_case {
	int policy;
	const char *desc;
	unsigned int flags;
	int ret;
	int err;
	char *addr;
	int (*pre_test)(struct test_case *tc);
	void (*alloc)(struct test_case *tc);
	struct bitmask **exp_nodemask;
};

static void test_set_mempolicy_default(struct test_case *tc);
static void test_set_mempolicy_none(struct test_case *tc);
static int test_mbind_none(struct test_case *tc);
static int test_mbind_default(struct test_case *tc);
static int set_invalid_addr(struct test_case *tc);

static struct test_case tcase[] = {
	{
		POLICY_DESC_TEXT(MPOL_DEFAULT, "no target"),
		.ret = 0,
		.err = 0,
		.alloc = test_set_mempolicy_none,
		.exp_nodemask = &empty_nodemask,
	},
	{
		POLICY_DESC(MPOL_BIND),
		.ret = 0,
		.err = 0,
		.alloc = test_set_mempolicy_default,
	},
	{
		POLICY_DESC(MPOL_INTERLEAVE),
		.ret = 0,
		.err = 0,
		.alloc = test_set_mempolicy_default,
		.exp_nodemask = &nodemask,
	},
	{
		POLICY_DESC_TEXT(MPOL_PREFERRED, "no target"),
		.ret = 0,
		.err = 0,
		.alloc = test_set_mempolicy_none,
		.exp_nodemask = &nodemask,
	},
	{
		.alloc = test_set_mempolicy_default,
		POLICY_DESC(MPOL_PREFERRED),
		.ret = 0,
		.err = 0,
	},
	{
		POLICY_DESC_FLAGS_TEXT(MPOL_DEFAULT, MPOL_F_ADDR, "no target"),
		.ret = 0,
		.err = 0,
		.pre_test = test_mbind_none,
		.alloc = test_set_mempolicy_none,
		.exp_nodemask = &nodemask,
	},
	{
		POLICY_DESC_FLAGS(MPOL_BIND, MPOL_F_ADDR),
		.pre_test = test_mbind_default,
		.ret = 0,
		.err = 0,
	},
	{
		POLICY_DESC_FLAGS(MPOL_INTERLEAVE, MPOL_F_ADDR),
		.ret = 0,
		.err = 0,
		.pre_test = test_mbind_default,
		.alloc = test_set_mempolicy_default,
		.exp_nodemask = &nodemask,
	},
	{
		POLICY_DESC_FLAGS_TEXT(MPOL_PREFERRED, MPOL_F_ADDR, "no target"),
		.ret = 0,
		.err = 0,
		.pre_test = test_mbind_none,
		.alloc = test_set_mempolicy_none,
		.exp_nodemask = &nodemask,
	},
	{
		POLICY_DESC_FLAGS(MPOL_PREFERRED, MPOL_F_ADDR),
		.ret = 0,
		.err = 0,
		.pre_test = test_mbind_default,
		.alloc = test_set_mempolicy_default,
	},
	{
		.flags = MPOL_F_ADDR,
		POLICY_DESC_TEXT(MPOL_DEFAULT, "invalid address"),
		.ret = -1,
		.err = EFAULT,
		.pre_test = set_invalid_addr,
		.exp_nodemask = &nodemask,
	},
	{
		POLICY_DESC_TEXT(MPOL_DEFAULT, "invalid flags, no target"),
		.flags = -1,
		.ret = -1,
		.err = EINVAL,
		.alloc = test_set_mempolicy_none,
		.exp_nodemask = &nodemask,
	},
};

static void test_set_mempolicy_default(struct test_case *tc)
{
	TEST(set_mempolicy(tc->policy, nodemask->maskp, nodemask->size));
}

static void test_set_mempolicy_none(struct test_case *tc)
{
	TEST(set_mempolicy(tc->policy, NULL, 0));
}

static int test_mbind(struct test_case *tc, unsigned long *maskp, unsigned long size)
{
	tc->addr = SAFE_MMAP(NULL, MEM_LENGTH, PROT_READ | PROT_WRITE,
			     MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);

	TEST(mbind(tc->addr, MEM_LENGTH, tc->policy, maskp, size, 0));

	if (TST_RET < 0) {
		tst_res(TFAIL | TERRNO, "mbind() failed");
		return -1;
	}

	return 0;
}

static int test_mbind_none(struct test_case *tc)
{
	return test_mbind(tc, NULL, 0);
}

static int test_mbind_default(struct test_case *tc)
{
	return test_mbind(tc, nodemask->maskp, nodemask->size);
}

static int set_invalid_addr(struct test_case *tc)
{
	int ret;

	ret = test_mbind_none(tc);
	tc->addr = NULL;

	return ret;
}

static void setup(void)
{
	unsigned int i;

	node = tst_get_nodemap(TST_NUMA_MEM, PAGES_ALLOCATED * getpagesize() / 1024);
	if (node->cnt < 1)
		tst_brk(TCONF, "test requires at least one NUMA memory node");

	nodemask = numa_allocate_nodemask();
	getnodemask = numa_allocate_nodemask();
	numa_bitmask_setbit(nodemask, node->map[0]);

	for (i = 0; i < ARRAY_SIZE(tcase); i++) {
		struct test_case *tc = &tcase[i];
		tst_res(TINFO, "setup: %d", i); // FIXME: debug

		if (tc->pre_test)
			if (tc->pre_test(tc) == -1)
				continue;

		if (tc->alloc) {
			tc->alloc(tc);

			if (TST_RET < 0) {
				tst_res(TFAIL | TERRNO, "allocation failed");

				if (tc->pre_test)
					SAFE_MUNMAP(tc->addr, MEM_LENGTH);
				continue;
			}
		}
	}
}

static void cleanup(void)
{
	tst_nodemap_free(node);
}

static void do_test(unsigned int i)
{
	struct test_case *tc = &tcase[i];
	int policy;

	tst_res(TINFO, "test #%d: %s", (i+1), tc->desc);

	TEST(get_mempolicy(&policy, getnodemask->maskp, getnodemask->size,
			   tc->addr, tc->flags));

	if (TST_RET == -1 && tc->policy == MPOL_DEFAULT)
		numa_bitmask_clearall(nodemask);

	if (tc->pre_test)
		SAFE_MUNMAP(tc->addr, MEM_LENGTH);

	if (tc->ret != TST_RET) {
		tst_res(TFAIL, "wrong return code: %ld, expected: %d",
			TST_RET, tc->ret);
		return;
	}
	if (TST_RET == -1 && TST_ERR != tc->err) {
		tst_res(TFAIL | TTERRNO, "expected errno: %s, got",
			tst_strerrno(tc->err));
		return;
	}

	tst_res(TPASS, "returned expected return value and errno");
}

static struct tst_test test = {
	.tcnt = ARRAY_SIZE(tcase),
	.test = do_test,
	.setup = setup,
	.cleanup = cleanup,
};

#else
TST_TEST_TCONF(NUMA_ERROR_MSG);
#endif /* HAVE_NUMA_V2 */
