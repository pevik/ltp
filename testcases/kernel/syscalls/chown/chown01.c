// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2000 Silicon Graphics, Inc.  All Rights Reserved.
 * AUTHOR: William Roske
 * CO-PILOT: Dave Fenner
 */

/*\
 * [Description]
 *
 * This is a Phase I test for the chown(2) system call. It is intended
 * to provide a limited exposure of the system call, for now. It
 * should/will be extended when full functional tests are written for
 * chown(2).
 *
 * [Algorithm]
 *
 * - Execute system call
 * - Check return code, if system call failed (return=-1)
 * -   Log the errno and Issue a FAIL message
 * - Otherwise, Issue a PASS message
 */

#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#include "tst_test.h"
#include "compat_tst_16.h"

char fname[255];
int uid, gid;

static void run(void)
{
	TEST(CHOWN(fname, uid, gid));

	if (TST_RET == -1)
		tst_res(TFAIL | TTERRNO, "chown(%s, %d,%d) failed",
			fname, uid, gid);
	else
		tst_res(TPASS, "chown(%s, %d,%d) returned %ld",
			fname, uid, gid, TST_RET);
}

static void setup(void)
{
	UID16_CHECK((uid = geteuid()), "chown");
	GID16_CHECK((gid = getegid()), "chown");
	sprintf(fname, "t_%d", getpid());
	SAFE_FILE_PRINTF(fname, "davef");
}

static struct tst_test test = {
	.needs_tmpdir = 1,
	.setup = setup,
	.test_all = run,
};

