// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2024 Wei Gao <wegao@suse.com>
 */

/*\
 * [Description]
 *
 * Test for kernel commit 5d1f903f75a80daa4dfb3d84e114ec8ecbf29956
 * "attr: block mode changes of symlinks".
 *
 */

#include "lapi/fcntl.h"
#include "tst_test.h"

#define MODE 0644
#define TESTFILE "testfile"
#define TESTFILE_SYMLINK "testfile_symlink"

static void run(void)
{
	struct stat stat_file, stat_sym;
	int mode = 0;
	char fd_path[100];

	int fd = SAFE_OPEN(TESTFILE_SYMLINK, O_PATH | O_NOFOLLOW);

	sprintf(fd_path, "/proc/self/fd/%d", fd);

	TST_EXP_FAIL(chmod(fd_path, mode), ENOTSUP, "chmod(%s, %04o)",
			TESTFILE_SYMLINK, mode);

	SAFE_STAT(TESTFILE, &stat_file);
	SAFE_LSTAT(TESTFILE_SYMLINK, &stat_sym);

	stat_file.st_mode &= ~S_IFREG;
	stat_sym.st_mode &= ~S_IFLNK;

	if (stat_file.st_mode == (unsigned int)mode) {
		tst_res(TFAIL, "stat(%s) mode=%04o",
				TESTFILE, stat_file.st_mode);
	} else {
		tst_res(TPASS, "stat(%s) mode=%04o",
				TESTFILE, stat_file.st_mode);
	}

	if (stat_sym.st_mode == (unsigned int)mode) {
		tst_res(TFAIL, "stat(%s) mode=%04o",
				TESTFILE_SYMLINK, stat_sym.st_mode);
	} else {
		tst_res(TPASS, "stat(%s) mode=%04o",
				TESTFILE_SYMLINK, stat_sym.st_mode);
	}

	SAFE_CLOSE(fd);
}

static void setup(void)
{
	SAFE_TOUCH(TESTFILE, MODE, NULL);
	SAFE_SYMLINK(TESTFILE, TESTFILE_SYMLINK);
}

static struct tst_test test = {
	.setup = setup,
	.test_all = run,
	.needs_tmpdir = 1,
	.min_kver = "6.6",
};
