// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2005-2006 IBM Corporation.
 * Author: David Gibson & Adam Litke
 */

/*\
 * [Description]
 *
 * Test case is used to verify the correct functionality
 * and compatibility of the library with the "truncate" system call when
 * operating on files residing in a mounted huge page filesystem.
 */

#include "hugetlb.h"
#include <setjmp.h>
#include <signal.h>

#define MNTPOINT "hugetlbfs/"

static long hpage_size;
static int fd;
static sigjmp_buf sig_escape;
static volatile int test_pass;
static int sigbus_count;

static void sigbus_handler(int signum LTP_ATTRIBUTE_UNUSED)
{
	test_pass = 1;
	siglongjmp(sig_escape, 17);
}

static void run_test(void)
{
	void *p;
	volatile unsigned int *q;

	sigbus_count = 0;
	test_pass = 0;

	struct sigaction my_sigaction;

	my_sigaction.sa_handler = sigbus_handler;
	p = SAFE_MMAP(NULL, hpage_size, PROT_READ|PROT_WRITE, MAP_SHARED,
			fd, 0);
	q = p;
	*q = 0;
	SAFE_SIGACTION(SIGBUS, &my_sigaction, NULL);
	SAFE_FTRUNCATE(fd, 0);

	if (sigsetjmp(sig_escape, 1) == 0)
		*q;
	else
		sigbus_count++;

	if (sigbus_count != 1)
		tst_res(TFAIL, "Didn't SIGBUS");

	if (test_pass == 1)
		tst_res(TPASS, "Expected SIGBUS triggered");

	SAFE_MUNMAP(p, hpage_size);
}


static void setup(void)
{
	hpage_size = tst_get_hugepage_size();
	fd = tst_creat_unlinked(MNTPOINT, 0);
}

static void cleanup(void)
{
	if (fd > 0)
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
