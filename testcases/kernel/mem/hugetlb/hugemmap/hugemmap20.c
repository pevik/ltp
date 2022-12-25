// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2005-2006 David Gibson & Adam Litke, IBM Corporation.
 * Author: David Gibson & Adam Litke
 */

/*\
 * [Description]
 *
 * The test checks that mlocking hugetlb areas works with all combinations
 * of MAP_PRIVATE and MAP_SHARED with and without MAP_LOCKED specified.
 */

#include <stdio.h>
#include <sys/mount.h>
#include <limits.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/resource.h>

#include "hugetlb.h"

#define MNTPOINT "hugetlbfs/"
static int  fd = -1;
static unsigned long hpage_size;

static void test_simple_mlock(int flags, char *flags_str)
{
	void *p;
	int ret;

	fd = tst_creat_unlinked(MNTPOINT, 0);
	p = SAFE_MMAP(0, hpage_size, PROT_READ|PROT_WRITE, flags, fd, 0);

	ret = mlock(p, hpage_size);
	if (ret) {
		tst_res(TFAIL|TERRNO, "mlock() failed (flags %s)", flags_str);
		goto cleanup;
	}

	ret = munlock(p, hpage_size);
	if (ret)
		tst_res(TFAIL|TERRNO, "munlock() failed (flags %s)", flags_str);
	else
		tst_res(TPASS, "mlock/munlock with %s hugetlb mmap",
				flags_str);
cleanup:
	SAFE_MUNMAP(p, hpage_size);
	SAFE_CLOSE(fd);
}

static void run_test(void)
{
	struct rlimit limit_info;

	if (getrlimit(RLIMIT_MEMLOCK, &limit_info))
		tst_res(TWARN|TERRNO, "Unable to read locked memory rlimit");
	if (limit_info.rlim_cur < hpage_size)
		tst_brk(TCONF, "Locked memory ulimit set below huge page size");

	test_simple_mlock(MAP_PRIVATE, "MAP_PRIVATE");
	test_simple_mlock(MAP_SHARED, "MAP_SHARED");
	test_simple_mlock(MAP_PRIVATE|MAP_LOCKED, "MAP_PRIVATE|MAP_LOCKED");
	test_simple_mlock(MAP_SHARED|MAP_LOCKED, "MAP_SHARED|MAP_LOCKED");

}

static void setup(void)
{
	hpage_size = SAFE_READ_MEMINFO(MEMINFO_HPAGE_SIZE)*1024;
}

static void cleanup(void)
{
	if (fd >= 0)
		SAFE_CLOSE(fd);
}

static struct tst_test test = {
	.needs_root = 1,
	.mntpoint = MNTPOINT,
	.needs_hugetlbfs = 1,
	.needs_tmpdir = 1,
	.setup = setup,
	.cleanup = cleanup,
	.test_all = run_test,
	.hugepages = {1, TST_NEEDS},
};
