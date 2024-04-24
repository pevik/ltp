// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2000 Silicon Graphics, Inc.  All Rights Reserved.
 * Authors: David Fenner, Jon Hendrickson
 * Copyright (C) 2024 Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * This test verifies that rename() is working correctly on symlink()
 * generated files.
 */

#include <stdlib.h>
#include "tst_test.h"

#define OBJPATH "object"
#define OLDNAME "msymlink0"
#define NEWNAME "asymlink0"

static char *tmpdir;

static void test_existing(void)
{
	tst_res(TINFO, "Test rename() on symlink pointing to an existent path");

	struct stat oldsym_stat;
	struct stat newsym_stat;

	SAFE_SYMLINK(tmpdir, OLDNAME);
	SAFE_STAT(OLDNAME, &oldsym_stat);

	SAFE_RENAME(OLDNAME, NEWNAME);
	SAFE_STAT(NEWNAME, &newsym_stat);

	TST_EXP_EQ_LI(oldsym_stat.st_ino, newsym_stat.st_ino);
	TST_EXP_EQ_LI(oldsym_stat.st_dev, newsym_stat.st_dev);

	SAFE_UNLINK(NEWNAME);
}

static void test_non_existing(void)
{
	tst_res(TINFO, "Test rename() on symlink pointing to a non-existent path");

	struct stat path_stat;

	SAFE_SYMLINK("this_path_doesnt_exist", OLDNAME);
	TST_EXP_FAIL(stat(OLDNAME, &path_stat), ENOENT);

	SAFE_RENAME(OLDNAME, NEWNAME);
	TST_EXP_FAIL(stat(NEWNAME, &path_stat), ENOENT);

	SAFE_UNLINK(NEWNAME);
}

static void test_creat(void)
{
	tst_res(TINFO, "Test rename() on symlink pointing to a path created lately");

	struct stat path_stat;
	int fd;

	SAFE_SYMLINK(OBJPATH, OLDNAME);
	TST_EXP_FAIL(stat(OLDNAME, &path_stat), ENOENT);

	tst_res(TINFO, "Create object file");

	fd = SAFE_CREAT(OBJPATH, 0700);
	if (fd >= 0)
		SAFE_CLOSE(fd);

	SAFE_RENAME(OLDNAME, NEWNAME);
	TST_EXP_PASS(stat(NEWNAME, &path_stat));

	SAFE_UNLINK(OBJPATH);
	SAFE_UNLINK(NEWNAME);
}

static void run(void)
{
	test_existing();
	test_creat();
	test_non_existing();
}

static void setup(void)
{
	tmpdir = tst_tmpdir_path();
}

static struct tst_test test = {
	.setup = setup,
	.test_all = run,
	.needs_tmpdir = 1,
};
