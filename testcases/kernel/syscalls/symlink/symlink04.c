// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines  Corp., 2001
 * Copyright (c) Linux Test Project, 2003-2023
 * Author: 07/2001 John George
 */

/*\
 * [Description]
 *
 * Check that a symbolic link may point to an existing file or
 * to a nonexistent one.
 */

#include "tst_test.h"
#include <stdlib.h>
#include <stdio.h>

#define TESTFILE    "testfile"
#define NONFILE     "noexistfile"
#define SYMFILE     "slink_file"

static int fd;
static char *testfile;
static char *nonfile;

static struct tcase {
	char **srcfile;
} tcases[] = {
	{&testfile},
	{&nonfile},
};

static void setup(void)
{
	fd = SAFE_OPEN(TESTFILE, O_RDWR | O_CREAT, 0644);
}

static void verify_symlink(unsigned int i)
{
	struct tcase *tc = &tcases[i];

	struct stat stat_buf;

	TST_EXP_PASS(symlink(*tc->srcfile, SYMFILE), "symlink(%s, %s)",
		     *tc->srcfile, SYMFILE);

	SAFE_LSTAT(SYMFILE, &stat_buf);

	if (!S_ISLNK(stat_buf.st_mode))
		tst_res(TFAIL, "symlink of %s doesn't exist", *tc->srcfile);

	SAFE_UNLINK(SYMFILE);
}

static void cleanup(void)
{
	if (fd > -1)
		SAFE_CLOSE(fd);
}

static struct tst_test test = {
	.tcnt = ARRAY_SIZE(tcases),
	.setup = setup,
	.cleanup = cleanup,
	.test = verify_symlink,
	.bufs = (struct tst_buffers []) {
		{&testfile, .str = TESTFILE},
		{&nonfile, .str = NONFILE},
		{},
	},
	.needs_tmpdir = 1,
};
