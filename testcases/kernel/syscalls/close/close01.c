// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines  Corp., 2001
 * 07/2001 Ported by Wayne Boyer
 */

/*\
 * [Description]
 *
 * Test that closing a regular file and a pipe works correctly
 */

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "tst_test.h"
#include "tst_safe_macros.h"

char fname[40] = "";
int fild, newfd, pipefildes[2];

struct test_case_t {
	int *fd;
	char *type;
} tc[] = {
	/* file descriptor for a regular file */
	{&newfd, "file"},
	/* file descriptor for a pipe */
	{&pipefildes[0], "pipe"}
};

static void run(unsigned int i)
{
	fild = SAFE_CREAT(fname, 0777);
	newfd = SAFE_DUP(fild);
	SAFE_PIPE(pipefildes);

	TEST(close(*tc[i].fd));

	if (TST_RET == -1)
		tst_res(TFAIL, "call failed unexpectedly");

	if (close(*tc[i].fd) == -1)
		tst_res(TPASS, "%s appears closed", tc[i].type);
	else
		tst_res(TFAIL, "%s close succeeded on second attempt",
			tc[i].type);
}

static void setup(void)
{
	int mypid;

	umask(0);
	mypid = getpid();

	sprintf(fname, "fname.%d", mypid);
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

