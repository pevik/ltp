// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2000 Silicon Graphics, Inc.  All Rights Reserved.
 * Authors: David Fenner, Jon Hendrickson
 * Copyright (C) 2024 Andrea Cervesato andrea.cervesato@suse.com
 */

/*\
 * [Description]
 *
 * Verify that rename() is working correctly on symlink() generated files,
 * renaming symbolic link and checking if stat() information are preserved.
 */

#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include "tst_test.h"

#define OLDNAME "my_symlink0"
#define NEWNAME "asymlink"

static void run(void)
{
	struct stat oldsym_stat;
	struct stat newsym_stat;

	SAFE_SYMLINK(".", OLDNAME);
	SAFE_STAT(OLDNAME, &oldsym_stat);

	SAFE_RENAME(OLDNAME, NEWNAME);
	SAFE_STAT(NEWNAME, &newsym_stat);

	TST_EXP_EQ_LI(oldsym_stat.st_ino, newsym_stat.st_ino);
	TST_EXP_EQ_LI(oldsym_stat.st_dev, newsym_stat.st_dev);

	SAFE_UNLINK(NEWNAME);
}

static struct tst_test test = {
	.test_all = run,
	.needs_tmpdir = 1,
};
