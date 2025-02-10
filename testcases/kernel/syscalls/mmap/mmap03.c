// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines  Corp., 2001
 *               07/2001 Ported by Wayne Boyer
 * Copyright (C) 2025 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Map a file with mmap() syscall, creating a mapped region with execute access
 * under the following conditions:
 *
 * - file descriptor is open for read
 * - minimum file permissions should be 0555
 * - file being mapped has PROT_EXEC execute permission bit set
 *
 * mmap() should succeed returning the address of the mapped region
 * and the mapped region should contain the contents of the mapped file.
 *
 * With ia64 and PARISC/hppa, an attempt to access the contents of the
 * mapped region should rise signal SIGSEGV.
 */

#include "tst_test.h"

#define TEMPFILE "mmapfile"

static void *addr;
static char *tmpdata;
static size_t page_sz;
static int fdesc = -1;

static void run_child(void)
{
	char *data;

	tst_res(TINFO, "Map temporary file in memory with PROT_EXEC");

	addr = SAFE_MMAP(0, page_sz, PROT_EXEC,
		  MAP_FILE | MAP_SHARED, fdesc, 0);

	data = SAFE_MALLOC(page_sz);
	memset(data, 0, page_sz);

	tst_res(TINFO, "Read data back from mapped file");

	SAFE_READ(0, fdesc, data, page_sz);
	SAFE_LSEEK(fdesc, 0, SEEK_SET);

	TST_EXP_EQ_LI(memcmp(data, tmpdata, page_sz), 0);

	SAFE_MUNMAP(addr, page_sz);
	free(data);
}

static void run(void)
{
	pid_t pid;
	int status;

	pid = SAFE_FORK();
	if (!pid) {
		run_child();
		exit(0);
	}

	SAFE_WAITPID(pid, &status, 0);

#if defined(__ia64__) || defined(__hppa__) || defined(__mips__)
	if (WIFSIGNALED(status) && WTERMSIG(status) == SIGSEGV) {
		tst_res(TPASS, "SIGSEGV has been received");
	} else {
		tst_res(TFAIL, "Mapped memory region with NO "
			"access is accessible");
	}
#endif
}

static void setup(void)
{
	page_sz = getpagesize();

	tmpdata = SAFE_MALLOC(page_sz);
	memset(tmpdata, 'a', page_sz);

	tst_res(TINFO, "Create temporary file");

	tst_fill_file(TEMPFILE, 'a', page_sz, 1);
	fdesc = SAFE_OPEN(TEMPFILE, O_RDONLY, 0555);
}

static void cleanup(void)
{
	free(tmpdata);

	if (fdesc != -1)
		SAFE_CLOSE(fdesc);
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.cleanup = cleanup,
	.needs_tmpdir = 1,
	.forks_child = 1,
};
