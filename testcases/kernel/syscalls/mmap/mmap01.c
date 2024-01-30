// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines  Corp., 2001
 *	07/2001 Ported by Wayne Boyer
 * Copyright (c) 2023 SUSE LLC Avinesh Kumar <avinesh.kumar@suse.com>
 */

/*\
 * [Description]
 *
 * Verify that, mmap() succeeds when used to map a file where size of the
 * file is not a multiple of the page size, the memory area beyond the end
 * of the file to the end of the page is accessible. Also, verify that
 * this area is all zeroed and the modifications done to this area are
 * not written to the file.
 */

#include <stdlib.h>
#include "tst_test.h"

#define MNT_POINT	"mntpoint"
#define TEMPFILE	MNT_POINT"/mmapfile"

static int fd;
static size_t page_sz;
static size_t file_sz;
static char *addr;
static char *dummy;
static struct stat stat_buf;
static const char write_buf[] = "HelloWorld!";

static void setup(void)
{
	fd = SAFE_OPEN(TEMPFILE, O_RDWR | O_CREAT, 0666);

	SAFE_WRITE(SAFE_WRITE_ALL, fd, write_buf, strlen(write_buf));
	SAFE_LSEEK(fd, 0, SEEK_SET);
	SAFE_STAT(TEMPFILE, &stat_buf);

	file_sz = stat_buf.st_size;
	page_sz = getpagesize();

	dummy = SAFE_MALLOC(page_sz);
	memset(dummy, 0, page_sz);
}

static void run(void)
{
	char buf[20];

	addr = SAFE_MMAP(NULL, page_sz, PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, fd, 0);

	if (memcmp(&addr[file_sz], dummy, page_sz - file_sz) != 0) {
		tst_res(TFAIL, "mapped memory area contains invalid data");
		goto unmap;
	}

	addr[file_sz] = 'X';
	addr[file_sz + 1] = 'Y';
	addr[file_sz + 2] = 'Z';

	SAFE_MSYNC(addr, page_sz, MS_SYNC);

	SAFE_FILE_SCANF(TEMPFILE, "%s", buf);

	if (strcmp(write_buf, buf))
		tst_res(TFAIL, "File data has changed");
	else
		tst_res(TPASS, "mmap() functionality successful");

	SAFE_LSEEK(fd, 0, SEEK_SET);
	memset(&addr[file_sz], 0, 3);

unmap:
	SAFE_MUNMAP(addr, page_sz);
}

static void cleanup(void)
{
	if (fd > 0)
		SAFE_CLOSE(fd);

	free(dummy);
}

static struct tst_test test = {
	.setup = setup,
	.test_all = run,
	.cleanup = cleanup,
	.mntpoint = MNT_POINT,
	.mount_device = 1,
	.all_filesystems = 1
};
