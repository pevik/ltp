// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2019 SUSE LLC
 * Author: Christian Amann <camann@suse.com>
 */

/*
 * Copies the contents of one file into another and
 * checks if the timestamp gets updated in the process.
 */

#include "tst_test.h"
#include "copy_file_range.h"

static int fd_src;
static int fd_dest;

unsigned long getTimestamp(int fd)
{
	struct stat filestat;

	fstat(fd, &filestat);
	return filestat.st_mtime;
}

static void verify_copy_file_range_timestamp(void)
{
	loff_t offset;
	unsigned long timestamp, updated_timestamp;

	timestamp = getTimestamp(fd_dest);
	usleep(1000000);

	offset = 0;
	TEST(sys_copy_file_range(fd_src, &offset,
			fd_dest, 0, strlen(CONTENT), 0));
	if (TST_RET == -1) {
		tst_res(TFAIL, "copy_file_range unexpectedly failed!");
		return;
	}

	updated_timestamp = getTimestamp(fd_dest);

	if (timestamp == updated_timestamp) {
		tst_res(TFAIL, "copy_file_range did not update timestamp!");
		return;
	}

	tst_res(TPASS, "copy_file_range sucessfully updated the timestamp");
}

static void cleanup(void)
{
	if (fd_dest > 0)
		SAFE_CLOSE(fd_dest);
	if (fd_src  > 0)
		SAFE_CLOSE(fd_src);
}

static void setup(void)
{
	fd_dest = SAFE_OPEN(FILE_DEST_PATH, O_RDWR | O_CREAT, 0664);
	fd_src  = SAFE_OPEN(FILE_SRC_PATH,  O_RDWR | O_CREAT, 0664);
	SAFE_WRITE(1, fd_src,  CONTENT,  strlen(CONTENT));
	SAFE_CLOSE(fd_src);
	fd_src = SAFE_OPEN(FILE_SRC_PATH, O_RDONLY);
}


static struct tst_test test = {
	.test_all = verify_copy_file_range_timestamp,
	.setup = setup,
	.cleanup = cleanup,
	.min_kver = "4.5",
	.needs_tmpdir = 1,
};
