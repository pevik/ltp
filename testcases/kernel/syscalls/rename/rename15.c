// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2000 Silicon Graphics, Inc.  All Rights Reserved.
 *    Author: David Fenner
 *    Copilot: Jon Hendrickson
 * Copyright (C) 2024 Andrea Cervesato andrea.cervesato@suse.com
 */

/*\
 * [Description]
 *
 * This test verifies that rename() is working correctly on symlink()
 * generated files, renaming symbolic link and checking if stat() information
 * are preserved.
 */

#include "tst_test.h"

static void run(void)
{
	char *oldname = "my_symlink0";
	char *newname = "asymlink";
	struct stat oldsym_stat;
	struct stat newsym_stat;

	SAFE_SYMLINK(tst_get_tmpdir(), oldname);
	SAFE_STAT(oldname, &oldsym_stat);

	SAFE_RENAME(oldname, newname);
	SAFE_STAT(newname, &newsym_stat);

	TST_EXP_EQ_LI(oldsym_stat.st_ino, newsym_stat.st_ino);
	TST_EXP_EQ_LI(oldsym_stat.st_dev, newsym_stat.st_dev);

	SAFE_UNLINK(newname);
}

static struct tst_test test = {
	.test_all = run,
	.needs_tmpdir = 1,
};
