// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2000 Silicon Graphics, Inc.  All Rights Reserved.
 * AUTHOR: William Roske
 * CO-PILOT: Dave Fenner
 */

/*\
 * [Description]
 * Basic test for the close() syscall.
 *
 * [Algorithm]
 *
 * Call close() and expects it to succeed.
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include "tst_test.h"

#define FILENAME "close08_testfile"

static void run(void)
{
    int fd = SAFE_OPEN(FILENAME, O_RDWR | O_CREAT, 0700);
    TST_EXP_PASS(close(fd));
}

static struct tst_test test = {
        .needs_tmpdir = 1,
        .test_all = run,
};
