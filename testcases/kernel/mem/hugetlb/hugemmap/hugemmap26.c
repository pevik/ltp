// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2009 IBM Corporation.
 * Author: David Gibson
 */

/*\
 * [Description]
 *
 * Test Description: The kernel has bug for mremap() on some architecture.
 * mremap() can cause crashes on architectures with holes in the address
 * space (like ia64) and on powerpc with it's distinct page size "slices".
 *
 * This test get the huge mapping address and mremap() normal mapping
 * near to this huge mapping.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <sys/mount.h>
#include <limits.h>
#include <sys/param.h>
#include <sys/types.h>

#include "hugetlb.h"

#define RANDOM_CONSTANT	0x1234ABCD
#define MNTPOINT "hugetlbfs/"

static int  fd = -1;
static long hpage_size, page_size;

static int do_readback(void *p, size_t size, const char *stage)
{
	unsigned int *q = p;
	size_t i;

	tst_res(TINFO, "%s(%p, 0x%lx, \"%s\")", __func__, p,
	       (unsigned long)size, stage);

	for (i = 0; i < (size / sizeof(*q)); i++)
		q[i] = RANDOM_CONSTANT ^ i;

	for (i = 0; i < (size / sizeof(*q)); i++) {
		if (q[i] != (RANDOM_CONSTANT ^ i)) {
			tst_res(TFAIL, "Stage \"%s\": Mismatch at offset 0x%lx: 0x%x "
					"instead of 0x%lx", stage, i, q[i], RANDOM_CONSTANT ^ i);
			return -1;
		}
	}
	return 0;
}

static int do_remap(void *target)
{
	void *a, *b;
	int ret;

	a = SAFE_MMAP(NULL, page_size, PROT_READ|PROT_WRITE,
		  MAP_SHARED|MAP_ANONYMOUS, -1, 0);

	ret = do_readback(a, page_size, "base normal");
	if (ret)
		goto cleanup;
	b = mremap(a, page_size, page_size, MREMAP_MAYMOVE | MREMAP_FIXED,
		   target);

	if (b != MAP_FAILED) {
		do_readback(b, page_size, "remapped");
		a = b;
	} else
		tst_res(TINFO|TERRNO, "mremap(MAYMOVE|FIXED) disallowed");

cleanup:
	SAFE_MUNMAP(a, page_size);
	return ret;
}

static void run_test(void)
{
	void *p;
	int ret;

	fd = tst_creat_unlinked(MNTPOINT, 0);
	p = SAFE_MMAP(NULL, 3*hpage_size, PROT_READ|PROT_WRITE, MAP_SHARED,
		 fd, 0);

	SAFE_MUNMAP(p, hpage_size);

	SAFE_MUNMAP(p + 2*hpage_size, hpage_size);

	p = p + hpage_size;

	tst_res(TINFO, "Hugepage mapping at %p", p);

	ret = do_readback(p, hpage_size, "base hugepage");
	if (ret)
		goto cleanup;
	ret = do_remap(p - page_size);
	if (ret)
		goto cleanup;
	ret = do_remap(p + hpage_size);
	if (ret == 0)
		tst_res(TPASS, "Successfully tested mremap normal near hpage mapping");
cleanup:
	SAFE_MUNMAP(p, hpage_size);
	SAFE_CLOSE(fd);
}

static void setup(void)
{
	hpage_size = SAFE_READ_MEMINFO(MEMINFO_HPAGE_SIZE)*1024;
	page_size = getpagesize();
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
	.hugepages = {3, TST_NEEDS},
};
