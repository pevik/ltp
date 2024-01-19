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
 * This test verifies that utime() is working correctly on symlink()
 * generated files.
 */

#include <utime.h>
#include "tst_test.h"

static void test_utime(void)
{
	char *symname = "my_symlink0";
	struct stat oldsym_stat;
	struct stat newsym_stat;

	SAFE_SYMLINK(tst_get_tmpdir(), symname);
	SAFE_STAT(symname, &oldsym_stat);

	struct utimbuf utimes = {
		.actime = oldsym_stat.st_atime + 100,
		.modtime = oldsym_stat.st_mtime + 100
	};

	TST_EXP_PASS(utime(symname, &utimes));
	SAFE_STAT(symname, &newsym_stat);

	time_t temp, diff;

	temp = newsym_stat.st_atime - oldsym_stat.st_atime;
	diff = newsym_stat.st_mtime - oldsym_stat.st_mtime - temp;

	TST_EXP_EQ_LI(diff, 0);

	SAFE_UNLINK(symname);
}

static void test_utime_no_path(void)
{
	char *symname = "my_symlink1";
	struct utimbuf utimes;

	SAFE_SYMLINK("bc+eFhi!k", symname);
	TST_EXP_FAIL(utime(symname, &utimes), ENOENT);

	SAFE_UNLINK(symname);
}

static void test_utime_loop(void)
{
	char *symname = "my_symlink2";
	struct utimbuf utimes;

	SAFE_SYMLINK(symname, symname);
	TST_EXP_FAIL(utime(symname, &utimes), ELOOP);

	SAFE_UNLINK(symname);
}

static void run(void)
{
	test_utime();
	test_utime_no_path();
	test_utime_loop();
}

static struct tst_test test = {
	.test_all = run,
	.needs_tmpdir = 1,
};
