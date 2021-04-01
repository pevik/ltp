// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines  Corp., 2001
 *  07/2001 Ported by Wayne Boyer
 */

/*\
 * [Description]
 * Check that an invalid file descriptor returns EBADF
 *
 * [Algorithm]
 *
 * - call close using the TEST macro and passing in an invalid fd
 * - if the call succeedes
 * -   issue a FAIL message
 * - else
 * -   log the errno
 * -   if the errno == EBADF
 * -     issue a PASS message
 * -   else
 * -     issue a FAIL message
 */

#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>

#include "tst_test.h"
#include "tst_safe_macros.h"

static void run(void)
{
	TEST(close(-1));

	if (TST_RET != -1) {
		tst_res(TFAIL, "Closed a non existent fildes");
	} else {
		if (TST_ERR != EBADF) {
			tst_res(TFAIL, "close() FAILED to set errno "
				       "to EBADF on an invalid fd, got %d",
				TST_ERR);
		} else {
			tst_res(TPASS, "call returned EBADF");
		}
	}
}

static void setup(void)
{
	umask(0);
}

static struct tst_test test = {
	.setup = setup,
	.test_all = run,
};

