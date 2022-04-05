// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines  Corp., 2002
*  11/20/2002 Port to LTP <robbiew@us.ibm.com>
*  06/30/2001 Port to Linux	<nsharoff@us.ibm.com>
 * Copyright (C) 2022 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 * Copyright (c) 2022 Petr Vorel <pvorel@suse.cz>
 */

/*\
 * [Description]
 *
 * Test confstr(3) 700 (X/Open 7) functionality &ndash; POSIX 2008.
 */

#define _XOPEN_SOURCE 700

#include <stdlib.h>
#include <unistd.h>

#include "tst_test.h"

static struct test_case_t {
	int value;
	char *name;
} test_cases[] = {
	{ _CS_PATH, "PATH" },
	{ _CS_GNU_LIBC_VERSION, "GNU_LIBC_VERSION" },
	{ _CS_GNU_LIBPTHREAD_VERSION, "GNU_LIBPTHREAD_VERSION" },
	{ _CS_POSIX_V7_ILP32_OFF32_CFLAGS, "POSIX_V7_ILP32_OFF32_CFLAGS" },
	{ _CS_POSIX_V7_ILP32_OFF32_LDFLAGS, "POSIX_V7_ILP32_OFF32_LDFLAGS" },
	{ _CS_POSIX_V7_ILP32_OFF32_LIBS, "POSIX_V7_ILP32_OFF32_LIBS" },
	{ _CS_POSIX_V7_ILP32_OFFBIG_CFLAGS, "POSIX_V7_ILP32_OFFBIG_CFLAGS" },
	{ _CS_POSIX_V7_ILP32_OFFBIG_LDFLAGS, "POSIX_V7_ILP32_OFFBIG_LDFLAGS" },
	{ _CS_POSIX_V7_ILP32_OFFBIG_LIBS, "POSIX_V7_ILP32_OFFBIG_LIBS" },
	{ _CS_POSIX_V7_LP64_OFF64_CFLAGS, "POSIX_V7_LP64_OFF64_CFLAGS" },
	{ _CS_POSIX_V7_LP64_OFF64_LDFLAGS, "POSIX_V7_LP64_OFF64_LDFLAGS" },
	{ _CS_POSIX_V7_LP64_OFF64_LIBS, "POSIX_V7_LP64_OFF64_LIBS" },
	{ _CS_POSIX_V7_LPBIG_OFFBIG_CFLAGS, "POSIX_V7_LPBIG_OFFBIG_CFLAGS" },
	{ _CS_POSIX_V7_LPBIG_OFFBIG_LDFLAGS, "POSIX_V7_LPBIG_OFFBIG_LDFLAGS" },
	{ _CS_POSIX_V7_LPBIG_OFFBIG_LIBS, "POSIX_V7_LPBIG_OFFBIG_LIBS" },
#ifdef _CS_POSIX_V7_THREADS_CFLAGS
	{ _CS_POSIX_V7_THREADS_CFLAGS, "POSIX_V7_THREADS_CFLAGS" },
#endif
#ifdef _CS_POSIX_V7_THREADS_LDFLAGS
	{ _CS_POSIX_V7_THREADS_LDFLAGS, "POSIX_V7_THREADS_LDFLAGS" },
#endif
	{ _CS_POSIX_V7_WIDTH_RESTRICTED_ENVS, "POSIX_V7_WIDTH_RESTRICTED_ENVS" },
	{ _CS_V7_ENV, "V7_ENV" },
};

static void run(unsigned int i)
{
	char *buf;
	int len;

	TST_EXP_POSITIVE(confstr(test_cases[i].value, NULL, (size_t)0));

	if (!TST_PASS)
		return;

	len = TST_RET;
	buf = SAFE_MALLOC(len);

	TEST(confstr(test_cases[i].value, buf, len));

	if (buf[len - 1] != '\0') {
		tst_brk(TFAIL, "confstr: %s, %s", test_cases[i].name,
			tst_strerrno(TST_ERR));
	} else {
		tst_res(TPASS, "confstr %s = '%s'", test_cases[i].name, buf);
	}

	free(buf);
}

static struct tst_test test = {
	.test = run,
	.tcnt = ARRAY_SIZE(test_cases),
};
