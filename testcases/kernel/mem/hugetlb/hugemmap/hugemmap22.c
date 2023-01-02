// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2005-2006 IBM Corporation.
 * Author: David Gibson & Adam Litke
 */

/*\
 * [Description]
 *
 * This baseline test validates that a mapping of a certain size can be
 * created, correctly.  Once created, all the pages are filled with a
 * pattern and rechecked to test for corruption. The mapping is then
 * released. This process is repeated for a specified number of
 * iterations.
 */

#include <stdio.h>
#include <sys/mount.h>
#include <unistd.h>
#include <unistd.h>

#include "hugetlb.h"

#define ITERATIONS 10
#define NR_HUGEPAGES 2
#define MNTPOINT "hugetlbfs/"

static unsigned long hpage_size;
static int fd = -1;

static void test_per_iteration(size_t size, int iter)
{
	char *m;
	size_t i, j;
	char pattern = 'A';

	fd = tst_creat_unlinked(MNTPOINT, 0);
	m = SAFE_MMAP(NULL, size, (PROT_READ|PROT_WRITE), MAP_SHARED, fd, 0);

	for (i = 0; i < NR_HUGEPAGES; i++) {
		for (j = 0; j < hpage_size; j++) {
			if (*(m+(i*hpage_size)+j) != 0) {
				tst_res(TFAIL, "Iter %d: Verifying the mmap area failed. "
				     "Got %c, expected 0", iter,
				     *(m+(i*hpage_size)+j));
				goto cleanup;
			}
		}
	}
	for (i = 0; i < NR_HUGEPAGES; i++) {
		pattern = 65+(i%26);
		memset(m+(i*hpage_size), pattern, hpage_size);
	}

	for (i = 0; i < NR_HUGEPAGES; i++) {
		pattern = 65+(i%26);
		for (j = 0; j < hpage_size; j++) {
			if (*(m+(i*hpage_size)+j) != pattern) {
				tst_res(TFAIL, "Verifying the mmap area failed. "
				     "got: %c, expected: %c",
				     *(m+(i*hpage_size)+j), pattern);
				goto cleanup;
			}
		}
	}

cleanup:
	SAFE_MUNMAP(m, size);
	SAFE_CLOSE(fd);
}

static void run_test(void)
{
	size_t size;
	int i;

	size = NR_HUGEPAGES * hpage_size;
	for (i = 0; i < ITERATIONS; i++)
		test_per_iteration(size, i);

	tst_res(TPASS, "Successfully verified the mmap area.");
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
	.hugepages = {NR_HUGEPAGES, TST_NEEDS},
};
