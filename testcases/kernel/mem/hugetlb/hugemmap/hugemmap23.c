// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2005-2006 IBM Corporation.
 * Author: David Gibson & Adam Litke
 */

/*\
 * [Description]
 *
 * This test uses mprotect to change protection of hugepage mapping and
 * perform read/write operation. It checks if the operation results in
 * expected behaviour as per the protection.
 */

#include <stdio.h>
#include <sys/mount.h>
#include <unistd.h>
#include <sys/mman.h>
#include <setjmp.h>
#include <signal.h>

#include "hugetlb.h"

#ifndef barrier
# ifdef mb
	/* Redefining the mb() */
#   define barrier() mb()
# else
#   define barrier() __asm__ __volatile__ ("" : : : "memory")
# endif
#endif

#define MNTPOINT "hugetlbfs/"
#define RANDOM_CONSTANT	0x1234ABCD

static int  fd = -1;
static sigjmp_buf sig_escape;
static void *sig_expected = MAP_FAILED;
static long hpage_size;

static void sig_handler(int signum, siginfo_t *si, void *uc)
{
	(void)uc;
	if (signum == SIGSEGV) {
		tst_res(TINFO, "SIGSEGV at %p (sig_expected=%p)", si->si_addr,
		       sig_expected);
		if (si->si_addr == sig_expected)
			siglongjmp(sig_escape, 1);
		tst_res(TFAIL, "SIGSEGV somewhere unexpected");
	} else
		tst_res(TFAIL, "Unexpected signal %s", strsignal(signum));
}

static int test_read(void *p)
{
	volatile unsigned long *pl = p;
	unsigned long x;

	if (sigsetjmp(sig_escape, 1)) {
		/* We got a SEGV */
		sig_expected = MAP_FAILED;
		return -1;
	}

	sig_expected = p;
	barrier();
	x = *pl;
	tst_res(TINFO, "Read back %lu", x);
	barrier();
	sig_expected = MAP_FAILED;
	return 0;
}

static int test_write(void *p, unsigned long val)
{
	volatile unsigned long *pl = p;
	unsigned long x;

	if (sigsetjmp(sig_escape, 1)) {
		/* We got a SEGV */
		sig_expected = MAP_FAILED;
		return -1;
	}

	sig_expected = p;
	barrier();
	*pl = val;
	x = *pl;
	barrier();
	sig_expected = MAP_FAILED;

	return (x != val);
}

static int test_prot(void *p, int prot, char *prot_str)
{
	int r, w;

	r = test_read(p);
	tst_res(TINFO, "On Read: %d", r);
	w = test_write(p, RANDOM_CONSTANT);
	tst_res(TINFO, "On Write: %d", w);

	if (prot & PROT_READ) {
		if (r != 0) {
			tst_res(TFAIL, "read failed on mmap(prot %s)", prot_str);
			return -1;
		}

	} else {
		if (r != -1) {
			tst_res(TFAIL, "read succeeded on mmap(prot %s)", prot_str);
			return -1;
		}
	}

	if (prot & PROT_WRITE) {
		switch (w) {
		case -1:
			tst_res(TFAIL, "write failed on mmap(prot %s)", prot_str);
			return -1;
		case 0:
			break;
		case 1:
			tst_res(TFAIL, "write mismatch on mmap(prot %s)", prot_str);
			return -1;
		default:
			tst_res(TWARN, "Bug in test");
			return -1;
		}
	} else {
		switch (w) {
		case -1:
			break;
		case 0:
			tst_res(TFAIL, "write succeeded on mmap(prot %s)", prot_str);
			return -1;
		case 1:
			tst_res(TFAIL, "write mismatch on mmap(prot %s)", prot_str);
			return -1;
		default:
			tst_res(TWARN, "Bug in test");
			break;
		}
	}
	return 0;
}

static int test_mprotect(int fd, char *testname,
			  unsigned long len1, int prot1, char *prot1_str,
			  unsigned long len2, int prot2, char *prot2_str)
{
	void *p;
	int ret;

	tst_res(TINFO, "Testing %s", testname);
	tst_res(TINFO, "Mapping with prot %s", prot1_str);
	p = SAFE_MMAP(NULL, len1, prot1, MAP_SHARED, fd, 0);

	ret = test_prot(p, prot1, prot1_str);
	if (ret)
		goto cleanup;
	tst_res(TINFO, "mprotect()ing to prot %s", prot2_str);
	ret = mprotect(p, len2, prot2);
	if (ret != 0) {
		tst_res(TFAIL|TERRNO, "%s: mprotect(prot %s)", testname, prot2_str);
		goto cleanup;
	}

	ret = test_prot(p, prot2, prot2_str);
	if (ret)
		goto cleanup;
	if (len2 < len1)
		ret = test_prot(p + len2, prot1, prot1_str);

cleanup:
	SAFE_MUNMAP(p, len1);
	return ret;
}

static void run_test(void)
{
	void *p;

	struct sigaction sa = {
		.sa_sigaction = sig_handler,
		.sa_flags = SA_SIGINFO,
	};

	SAFE_SIGACTION(SIGSEGV, &sa, NULL);

	fd = tst_creat_unlinked(MNTPOINT, 0);
	p = SAFE_MMAP(NULL, 2*hpage_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	memset(p, 0, hpage_size);
	SAFE_MUNMAP(p, hpage_size);

	if (test_mprotect(fd, "R->RW", hpage_size, PROT_READ, "PROT_READ",
		      hpage_size, PROT_READ|PROT_WRITE, "PROT_READ|PROT_WRITE"))
		goto cleanup;
	if (test_mprotect(fd, "RW->R", hpage_size, PROT_READ|PROT_WRITE,
		     "PROT_READ|PROT_WRITE", hpage_size, PROT_READ, "PROT_READ"))
		goto cleanup;

	if (test_mprotect(fd, "R->RW 1/2", 2*hpage_size, PROT_READ, "PROT_READ",
		      hpage_size, PROT_READ|PROT_WRITE, "PROT_READ|PROT_WRITE"))
		goto cleanup;
	if (test_mprotect(fd, "RW->R 1/2", 2*hpage_size, PROT_READ|PROT_WRITE,
		      "PROT_READ|PROT_WRITE", hpage_size, PROT_READ, "PROT_READ"))
		goto cleanup;

	if (test_mprotect(fd, "NONE->R", hpage_size, PROT_NONE, "PROT_NONE",
		      hpage_size, PROT_READ, "PROT_READ"))
		goto cleanup;
	if (test_mprotect(fd, "NONE->RW", hpage_size, PROT_NONE, "PROT_NONE",
		      hpage_size, PROT_READ|PROT_WRITE, "PROT_READ|PROT_WRITE"))
		goto cleanup;

	tst_res(TPASS, "Successfully tested mprotect with hugetlb area");
cleanup:
	SAFE_MUNMAP(p+hpage_size, hpage_size);
	SAFE_CLOSE(fd);
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
	.hugepages = {2, TST_NEEDS},
};
