// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2000 Silicon Graphics, Inc.  All Rights Reserved.
 *  AUTHOR: William Roske
 *  CO-PILOT: Dave Fenner
 */

/*\
 * [Description]
 * Basic test for close(2)
 *
 * [Algorithm]
 *
 * - Execute system call
 * - Check return code, if system call failed (return=-1)
 * -   Log the errno and Issue a FAIL message.
 * - Otherwise
 * -   Issue a PASS message.
 */

#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#include "tst_test.h"
#include "tst_safe_macros.h"

char fname[255];
int fd;

static void run(void)
{
	SAFE_OPEN(fname, O_RDWR | O_CREAT, 0700);

	TEST(close(fd));

	if (TST_RET == -1)
		tst_res(TFAIL | TTERRNO, "close(%s) failed", fname);
	else
		tst_res(TPASS, "close(%s) returned %ld", fname, TST_RET);

	SAFE_UNLINK(fname);
}

void setup(void)
{
	sprintf(fname, "tfile_%d", getpid());
}

static struct tst_test test = {
	.setup = setup,
	.test_all = run,
};

