// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2021 FUJITSU LIMITED. All rights reserved.
 * Author: Yang Xu <xuyang2018.jy@cn.fujitsu.com>
 */

/*\
 * [DESCRIPTION]
 *
 * Basic mallinfo() test. Test the member of struct mallinfo
 * whether overflow when setting 2G size. mallinfo() is deprecated
 * since the type used for the fields is too small. We should use
 * mallinfo2() and it was added since glibc 2.33.
\*/

#include "tst_test.h"
#include "tst_safe_macros.h"
#include "tst_mallinfo.h"

#ifdef HAVE_MALLINFO

void test_mallinfo(void)
{
	struct mallinfo info;
	char *buf;
	size_t size = 2UL * 1024UL * 1024UL * 1024UL;

	buf = malloc(size);
	if (!buf) {
		tst_res(TCONF, "Current system can not malloc 2G space, skip it");
		return;
	}
	info = mallinfo();
	if (info.hblkhd < 0) {
		tst_print_mallinfo("Test malloc 2G", &info);
		tst_res(TFAIL, "The member of struct mallinfo overflow, we should use mallinfo2");
	} else {
		/*We will never get here*/
		tst_res(TPASS, "The member of struct mallinfo doesn't overflow");
	}

	free(buf);
}

static struct tst_test test = {
	.test_all = test_mallinfo,
};

#else
TST_TEST_TCONF("system doesn't implement non-POSIX mallinfo()");
#endif
