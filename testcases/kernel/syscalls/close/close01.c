// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines  Corp., 2001
 * 07/2001 Ported by Wayne Boyer
 */

/*\
 * [Description]
 *
 * Basic test for the close() syscall.
 *
 * [Description]
 *
 * Test that closing a regular file and a pipe works correctly.
 */

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "tst_test.h"

#define FILENAME "close01_testfile"

int fild, newfd, pipefildes[2];

static void setup_file(void)
{
	newfd = SAFE_DUP(fild);
}

static void setup_pipe(void)
{
	SAFE_PIPE(pipefildes);
	SAFE_CLOSE(pipefildes[1]);
}

struct test_case_t {
	int *fd;
	char *type;
	void (*setupfunc) ();
} tc[] = {
	{&newfd, "file", setup_file},
	{&pipefildes[0], "pipe", setup_pipe}
};

static void run(unsigned int i)
{
	tc[i].setupfunc();
	TST_EXP_PASS(close(*tc[i].fd), "close a %s", tc[i].type);
}

static void setup(void)
{
	umask(0);
	fild = SAFE_CREAT(FILENAME, 0777);
}

static void cleanup(void)
{
	SAFE_CLOSE(fild);
}

static struct tst_test test = {
	.tcnt = ARRAY_SIZE(tc),
	.needs_tmpdir = 1,
	.setup = setup,
	.cleanup = cleanup,
	.test = run,
};
