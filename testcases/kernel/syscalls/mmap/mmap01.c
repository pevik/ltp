// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines  Corp., 2001
 * Copyright (c) 2024 Ricardo B. Marliere <rbm@suse.com>
 *
 * HISTORY
 *	07/2001 Ported by Wayne Boyer
 */

/*\
 * [Description]
 *
 * Verify that mmap() succeeds when used to map a file where size of the
 * file is not a multiple of the page size, the memory area beyond the end
 * of the file to the end of the page is accessible. Also, verify that
 * this area is all zeroed and the modifications done to this area are
 * not written to the file.
 *
 * mmap() should succeed returning the address of the mapped region.
 * The memory area beyond the end of file to the end of page should be
 * filled with zero.
 * The changes beyond the end of file should not get written to the file.
 */

#include "tst_test.h"

#define TEMPFILE "mmapfile"
#define STRING "hello world"

static char *addr;
static char *dummy;
static size_t page_sz;
static size_t file_sz;
static int fildes;

static void check_file(void)
{
	int i, fildes, buf_len = sizeof(STRING) + 3;
	char buf[buf_len];

	fildes = SAFE_OPEN(TEMPFILE, O_RDONLY);
	SAFE_READ(0, fildes, buf, sizeof(buf));

	for (i = 0; i < buf_len; i++)
		if (buf[i] == 'X' || buf[i] == 'Y' || buf[i] == 'Z')
			break;

	if (i == buf_len)
		tst_res(TPASS, "Functionality of mmap() successful");
	else
		tst_res(TFAIL, "Specified pattern found in file");

	SAFE_CLOSE(fildes);
}

static void run(void)
{
	pid_t pid;

	addr = SAFE_MMAP(NULL, page_sz, PROT_READ | PROT_WRITE,
			 MAP_FILE | MAP_SHARED, fildes, 0);

	/*
	 * Check if mapped memory area beyond EOF are zeros and changes beyond
	 * EOF are not written to file.
	 */
	if (memcmp(&addr[file_sz], dummy, page_sz - file_sz))
		tst_brk(TFAIL, "mapped memory area contains invalid data");

	/*
	 * Initialize memory beyond file size
	 */
	addr[file_sz] = 'X';
	addr[file_sz + 1] = 'Y';
	addr[file_sz + 2] = 'Z';

	/*
	 * Synchronize the mapped memory region with the file.
	 */
	if (msync(addr, page_sz, MS_SYNC) != 0) {
		tst_res(TFAIL | TERRNO, "failed to synchronize mapped file");
		return;
	}

	/*
	 * Now, search for the pattern 'XYZ' in the temporary file.
	 * The pattern should not be found and the return value should be 1.
	 */
	pid = SAFE_FORK();
	if (!pid) {
		check_file();
		exit(0);
	}

	SAFE_MUNMAP(addr, page_sz);
}

static void cleanup(void)
{
	if (dummy)
		free(dummy);
	if (fildes > 0)
		SAFE_CLOSE(fildes);
}

static void setup(void)
{
	char *write_buf = STRING;
	struct stat stat_buf;

	/* Create a temporary file used for mapping */
	fildes = SAFE_OPEN(TEMPFILE, O_RDWR | O_CREAT, 0666);

	/* Write some data into temporary file */
	SAFE_WRITE(SAFE_WRITE_ALL, fildes, write_buf, strlen(write_buf));

	/* Get the size of temporary file */
	SAFE_STAT(TEMPFILE, &stat_buf);
	file_sz = stat_buf.st_size;

	page_sz = getpagesize();

	/* Allocate and initialize dummy string of system page size bytes */
	dummy = SAFE_CALLOC(page_sz, sizeof(char));
}

static struct tst_test test = {
	.setup = setup,
	.cleanup = cleanup,
	.needs_tmpdir = 1,
	.test_all = run,
	.forks_child = 1,
};
