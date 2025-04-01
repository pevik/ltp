// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines  Corp., 2001
 *	07/2001 Ported by Wayne Boyer
 * Copyright (c) 2025 SUSE LLC Ricardo B. Marlière <rbm@suse.com>
 */

/*\
 * Verify that modify_ldt() calls:
 *
 * - Fails with EINVAL, when writing (func=1) to an invalid pointer
 * - Fails with EFAULT, when reading (func=0) from an invalid pointer
 * - Passes when reading (func=0) from a valid pointer
 */

#include "tst_test.h"

#ifdef __i386__
#include "common.h"

static void *ptr;
static char *buf;
static struct user_desc entry;
static struct tcase {
	int tfunc;
	void *ptr;
	unsigned long bytecount;
	int exp_errno;
} tcases[] = {
	{ 0, &ptr, sizeof(ptr), EFAULT },
	{ 0, &buf, sizeof(buf), 0 },
	{ 1, (void *)0, 0, EINVAL },
	{ 1, &entry, sizeof(struct user_desc) - 1, EINVAL },
	{ 1, &entry, sizeof(struct user_desc), EINVAL },
	{ 0x11, &entry, sizeof(struct user_desc), EINVAL },
};

void run(unsigned int i)
{
	struct tcase *tc = &tcases[i];

	if (tc->exp_errno)
		TST_EXP_FAIL(modify_ldt(tc->tfunc, tc->ptr, tc->bytecount),
			     tc->exp_errno);
	else
		TST_EXP_POSITIVE(modify_ldt(tc->tfunc, tc->ptr, tc->bytecount));
}

void setup(void)
{
	int seg[4];

	create_segment(seg, sizeof(seg));

	entry.contents = 3;
	entry.seg_not_present = 0;

	ptr = sbrk(0) + 0xFFF;
	tcases[0].ptr = ptr;
}

static struct tst_test test = {
	.test = run,
	.tcnt = ARRAY_SIZE(tcases),
	.setup = setup,
	.bufs =
		(struct tst_buffers[]){
			{ &buf, .size = sizeof(struct user_desc) },
			{},
		},
};

#else
TST_TEST_TCONF("Test supported only on i386");
#endif /* __i386__ */
