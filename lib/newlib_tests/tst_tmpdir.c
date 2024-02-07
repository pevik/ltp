// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2024 Petr Vorel <pvorel@suse.cz>
 */

/*
 * Tests TMPDIR variable cleanup.
 */

#include <stdlib.h>
#include "tst_defaults.h"
#include "tst_test.h"

static struct tcase {
	const char *orig;
	const char *fixed;
	const char *desc;
} tcases[] = {
	{NULL, TEMPDIR, "default value"},
	{"/tmp/", "/tmp", "removing trailing slash"},
	{"/var//tmp", "/var/tmp", "removing duplicate slashes"},
	{"//var///tmp///", "/var/tmp", "removing too many slashes"},
};

static void do_test(unsigned int nr)
{
	struct tcase *tc = &tcases[nr];
	const char *env_tmpdir;

	tst_res(TINFO, "Testing TMPDIR='%s' (%s)", tc->orig, tc->desc);

	if (tc->orig)
		SAFE_SETENV("TMPDIR", tc->orig, 1);

	env_tmpdir = tst_get_tmpdir_root();

	if (!env_tmpdir)
		tst_brk(TBROK, "Failed to get TMPDIR");

	if (!strcmp(tc->fixed, env_tmpdir))
		tst_res(TPASS, "TMPDIR '%s' is '%s'", env_tmpdir, tc->fixed);
	else
		tst_res(TFAIL, "TMPDIR '%s' should be '%s'", env_tmpdir, tc->fixed);
}

static struct tst_test test = {
	.test = do_test,
	.tcnt = ARRAY_SIZE(tcases),
};
