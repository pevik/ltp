// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * This test verifies that fchmodat2() syscall is properly working with
 * AT_SYMLINK_NOFOLLOW regular files, symbolic links and directories.
 */

#include "fchmodat2.h"
#include "lapi/fcntl.h"
#include "lapi/stat.h"

#define MNTPOINT "mntpoint"
#define FNAME "myfile"
#define SNAME "symlink"
#define DNAME "mydir"
#define DNAME_PATH MNTPOINT"/"DNAME

static int fd_dir = -1;

static void test_regular_file(void)
{
	tst_res(TINFO, "Using regular files");

	SAFE_CHMOD(MNTPOINT"/"FNAME, 0640);

	TST_EXP_PASS(fchmodat2(fd_dir, FNAME, 0700, 0));
	verify_mode(fd_dir, FNAME, S_IFREG | 0700);

	TST_EXP_PASS(fchmodat2(fd_dir, FNAME, 0700, AT_SYMLINK_NOFOLLOW));
	verify_mode(fd_dir, FNAME, S_IFREG | 0700);
}

static void test_symbolic_link(void)
{
	tst_res(TINFO, "Using symbolic link");

	TST_EXP_PASS(fchmodat2(fd_dir, SNAME, 0700, 0));
	verify_mode(fd_dir, FNAME, S_IFREG | 0700);
	verify_mode(fd_dir, SNAME, S_IFLNK | 0777);

	TST_EXP_FAIL(tst_syscall(__NR_fchmodat2, fd_dir, SNAME, 0640,
				 AT_SYMLINK_NOFOLLOW), EOPNOTSUPP);
}

static void test_empty_folder(void)
{
	tst_res(TINFO, "Using empty folder");

	int fd;

	SAFE_CHMOD(DNAME_PATH, 0640);
	fd = SAFE_OPEN(DNAME_PATH, O_PATH | O_DIRECTORY, 0640);

	TST_EXP_PASS(fchmodat2(fd, "", 0777, AT_EMPTY_PATH));
	verify_mode(fd_dir, DNAME, S_IFDIR | 0777);

	SAFE_CLOSE(fd);
}

static void run(void)
{
	test_regular_file();
	test_empty_folder();
	test_symbolic_link();
}

static void setup(void)
{
	fd_dir = SAFE_OPEN(MNTPOINT, O_PATH | O_DIRECTORY, 0640);

	if (access(DNAME_PATH, F_OK) == -1)
		SAFE_MKDIR(DNAME_PATH, 0640);

	SAFE_TOUCH(MNTPOINT"/"FNAME, 0640, NULL);
	SAFE_SYMLINKAT(FNAME, fd_dir, SNAME);
}

static void cleanup(void)
{
	SAFE_UNLINKAT(fd_dir, SNAME, 0);
	SAFE_RMDIR(DNAME_PATH);

	if (fd_dir != -1)
		SAFE_CLOSE(fd_dir);
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.cleanup = cleanup,
	.min_kver = "6.6",
	.mntpoint = MNTPOINT,
	.format_device = 1,
	.all_filesystems = 1,
	.tags = (const struct tst_tag[]) {
		{"linux-git", "5d1f903f75a8"},
		{}
	},
};
