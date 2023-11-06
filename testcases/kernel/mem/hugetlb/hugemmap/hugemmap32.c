// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023, IBM Corporation.
 * Author: Tarun Sahu
 */

/*\
 * [Description]
 *
 * Before kernel version 5.10-rc7, there was a bug that resulted in a "Bad Page
 * State" error when freeing gigantic hugepages. This happened because the
 * struct page entry compound_nr, which overlapped with page->mapping in the
 * first tail page, was not cleared, causing the error. To ensure that this
 * issue does not reoccur as struct page keeps changing and some fields are
 * managed by folio, this test checks that freeing gigantic hugepages does not
 * produce the above-mentioned error.
 */

#define _GNU_SOURCE
#include <dirent.h>
#include <stdio.h>

#include "hugetlb.h"

static void run_test(void)
{
	tst_res(TPASS, "If reserved & freed the gigantic page completely, then regard as pass");
}

static struct tst_test test = {
	.tags = (struct tst_tag[]) {
	    {"linux-git", "ba9c1201beaa"},
	    {"linux-git", "a01f43901cfb"},
	    {}
	},
	.needs_root = 1,
	.hugepages = {1, TST_NEEDS, TST_GIGANTIC},
	.test_all = run_test,
	.taint_check = TST_TAINT_B,
};
