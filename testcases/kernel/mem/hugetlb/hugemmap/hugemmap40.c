// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2005-2006 David Gibson & Adam Litke, IBM Corporation.
 * Copyright (C) 2006 Hugh Dickins <hugh@veritas.com>
 */

/*\
 * [Description]
 *
 * Test tries to allocate hugepages to cover a memory range that straddles the
 * 4GB boundary, using mmap(2) with and without MAP_FIXED.
 */

#define MAPS_BUF_SZ 4096
#define FOURGB (1UL << 32)
#define MNTPOINT "hugetlbfs/"

#include "hugetlb.h"

static unsigned long hpage_size;
static int fd = -1;
static unsigned long straddle_addr;

static int test_addr_huge(void *p)
{
	char name[256];
	char *dirend;
	int ret;
	struct statfs64 sb;

	ret = read_maps((unsigned long)p, name);
	if (ret < 0)
		return ret;

	if (ret == 0) {
		tst_res(TINFO, "Couldn't find address %p in /proc/self/maps", p);
		return -1;
	}

	/* looks like a filename? */
	if (name[0] != '/')
		return 0;

	/* Truncate the filename portion */

	dirend = strrchr(name, '/');
	if (dirend && dirend > name)
		*dirend = '\0';

	ret = statfs64(name, &sb);
	if (ret)
		return -1;

	return (sb.f_type == HUGETLBFS_MAGIC);
}

static void run_test(void)
{
	void *p;

	/* We first try to get the mapping without MAP_FIXED */
	tst_res(TINFO, "Mapping without MAP_FIXED at %lx...", straddle_addr);
	p = mmap((void *)straddle_addr, 2*hpage_size, PROT_READ|PROT_WRITE,
			MAP_SHARED, fd, 0);
	if (p == (void *)straddle_addr) {
		/* These tests irrelevant if we didn't get the straddle address*/
		if (test_addr_huge(p) != 1) {
			tst_brk(TFAIL, "1st Mapped address is not hugepage");
			goto windup;
		}
		if (test_addr_huge(p + hpage_size) != 1) {
			tst_brk(TFAIL, "2nd Mapped address is not hugepage");
			goto windup;
		}
		memset(p, 0, hpage_size);
		memset(p + hpage_size, 0, hpage_size);
		tst_res(TPASS, "Mapping without MAP_FIXED at %lx... completed", straddle_addr);
	} else {
		tst_res(TINFO, "Got %p instead, never mind, let's move to mapping with MAP_FIXED", p);
		munmap(p, 2*hpage_size);
	}

	tst_res(TINFO, "Mapping with MAP_FIXED at %lx...", straddle_addr);
	p = mmap((void *)straddle_addr, 2*hpage_size, PROT_READ|PROT_WRITE,
				MAP_SHARED|MAP_FIXED, fd, 0);

	if (p == MAP_FAILED) {
		/* this area crosses last low slice and first high slice */
		unsigned long below_start = FOURGB - 256L*1024*1024;
		unsigned long above_end = 1024L*1024*1024*1024;

		if (tst_mapping_in_range(below_start, above_end) == 1) {
			tst_res(TINFO, "region (4G-256M)-1T is not free");
			tst_res(TINFO, "mmap() failed: %s", strerror(errno));
			tst_res(TWARN, "Pass Inconclusive!");
			goto windup;
		} else
			tst_res(TFAIL, "mmap() FIXED failed: %s", strerror(errno));
	}

		if (p != (void *)straddle_addr) {
			tst_res(TINFO, "got %p instead", p);
			tst_res(TFAIL, "Wrong address with MAP_FIXED");
			goto windup;
		}

		if (test_addr_huge(p) != 1) {
			tst_brk(TFAIL, "1st Mapped address is not hugepage");
			goto windup;
		}

		if (test_addr_huge(p + hpage_size) != 1) {
			tst_brk(TFAIL, "2nd Mapped address is not hugepage");
			goto windup;
		}
		memset(p, 0, hpage_size);
		memset(p + hpage_size, 0, hpage_size);
		tst_res(TPASS, "Mapping with MAP_FIXED at %lx... completed", straddle_addr);

windup:
	SAFE_MUNMAP(p, 2*hpage_size);
}

static void setup(void)
{
	straddle_addr = FOURGB - hpage_size;
	hpage_size = tst_get_hugepage_size();
	fd = tst_creat_unlinked(MNTPOINT, 0, 0600);
	if (hpage_size > FOURGB)
		tst_brk(TCONF, "Huge page size is too large!");
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
	.hugepages = {2, TST_NEEDS},
	.setup = setup,
	.cleanup = cleanup,
	.test_all = run_test,
};
