// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2000 Silicon Graphics, Inc.  All Rights Reserved.
 * 06/1994 AUTHOR: Richard Logan CO-PILOT: William Roske
 * Copyright (c) 2023 SUSE LLC
 */

/*\
 * [Description]
 *
 * Basic test for dup(2) of a named pipe descriptor
 */

#include "tst_test.h"

#define Fname "dupfile"

static int fd;

static void run(void)
{
	TST_EXP_FD(dup(fd), "dup(%d)", fd);
	SAFE_CLOSE(TST_RET);
}

static void setup(void)
{
	fd = -1;

	SAFE_MKFIFO(Fname, 0777);
	fd = SAFE_OPEN(Fname, O_RDWR, 0700);
}

static void cleanup(void)
{
	if (fd != -1)
		SAFE_CLOSE(fd);
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.cleanup = cleanup,
	.needs_tmpdir = 1,
};
