// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2023 Wei Gao <wegao@suse.com>
 */

/*\
 * [Description]
 *
 * Verify basic fiemap ioctl
 *
 */

#include <linux/fs.h>
#include <linux/fiemap.h>
#include <stdlib.h>

#include "tst_test.h"

#define TESTFILE "testfile"
#define NUM_EXTENT 2
#define FILE_OFFSET ((rand() % 8 + 2) * getpagesize())

static char *buf;

static void print_extens(struct fiemap *fiemap)
{

	tst_res(TDEBUG, "File extent count: %u", fiemap->fm_mapped_extents);
	for (unsigned int i = 0; i < fiemap->fm_mapped_extents; ++i) {
		tst_res(TDEBUG, "Extent %u: Logical offset: %llu, Physical offset: %llu, flags: %u, Length: %llu",
				i + 1,
				fiemap->fm_extents[i].fe_logical,
				fiemap->fm_extents[i].fe_physical,
				fiemap->fm_extents[i].fe_flags,
				fiemap->fm_extents[i].fe_length);
	}
}

static void verify_ioctl(void)
{
	int fd;
	struct fiemap *fiemap;

	fd = SAFE_OPEN(TESTFILE, O_RDWR | O_CREAT, 0644);

	fiemap = SAFE_MALLOC(sizeof(struct fiemap) + sizeof(struct fiemap_extent) * NUM_EXTENT);
	fiemap->fm_start = 0;
	fiemap->fm_length = ~0ULL;
	fiemap->fm_extent_count = 1;

	fiemap->fm_flags =  -1;
	TST_EXP_FAIL(ioctl(fd, FS_IOC_FIEMAP, fiemap), EBADR);

	fiemap->fm_flags =  0;
	SAFE_IOCTL(fd, FS_IOC_FIEMAP, fiemap);
	print_extens(fiemap);
	if (fiemap->fm_mapped_extents == 0)
		tst_res(TPASS, "Check fiemap iotct zero fm_mapped_extents pass");
	else
		tst_res(TFAIL, "Check fiemap iotct zero fm_mapped_extents failed");

	SAFE_WRITE(SAFE_WRITE_ANY, fd, buf, getpagesize());
	SAFE_IOCTL(fd, FS_IOC_FIEMAP, fiemap);
	print_extens(fiemap);
	if (fiemap->fm_mapped_extents == 1 && fiemap->fm_extents[0].fe_physical == 0)
		tst_res(TPASS, "Check fiemap iotct one fm_mapped_extents pass");
	else
		tst_res(TFAIL, "Check fiemap iotct one fm_mapped_extents failed");

	fiemap->fm_flags = FIEMAP_FLAG_SYNC;
	SAFE_IOCTL(fd, FS_IOC_FIEMAP, fiemap);
	print_extens(fiemap);
	if ((fiemap->fm_mapped_extents == 1) &&
		(fiemap->fm_extents[0].fe_flags == FIEMAP_EXTENT_LAST) &&
		(fiemap->fm_extents[0].fe_physical > 0) &&
		(fiemap->fm_extents[0].fe_length == (__u64)getpagesize()))
		tst_res(TPASS, "Check fiemap iotct FIEMAP_FLAG_SYNC fm_flags pass");
	else
		tst_res(TFAIL, "Check fiemap iotct FIEMAP_FLAG_SYNC fm_flags failed");

	fiemap->fm_extent_count = NUM_EXTENT;
	srand(time(NULL));
	SAFE_LSEEK(fd, FILE_OFFSET, SEEK_SET);
	SAFE_WRITE(SAFE_WRITE_ALL, fd, buf, getpagesize());
	SAFE_IOCTL(fd, FS_IOC_FIEMAP, fiemap);
	print_extens(fiemap);
	if ((fiemap->fm_mapped_extents == NUM_EXTENT) &&
		(fiemap->fm_extents[NUM_EXTENT - 1].fe_flags == FIEMAP_EXTENT_LAST) &&
		(fiemap->fm_extents[NUM_EXTENT - 1].fe_physical > 0) &&
		(fiemap->fm_extents[NUM_EXTENT - 1].fe_length == (__u64)getpagesize()))
		tst_res(TPASS, "Check fiemap iotct multiple fm_mapped_extents pass");
	else
		tst_res(TFAIL, "Check fiemap iotct multiple fm_mapped_extents failed");

	free(fiemap);
	SAFE_CLOSE(fd);
	unlink(TESTFILE);
}

static void setup(void)
{
	buf = SAFE_MALLOC(getpagesize());
}

static void cleanup(void)
{
	free(buf);
}

static struct tst_test test = {
	.setup = setup,
	.cleanup = cleanup,
	.test_all = verify_ioctl,
	.needs_root = 1,
	.needs_tmpdir = 1,
	.skip_filesystems = (const char *const []) {
		"tmpfs",
		NULL
	},
};
