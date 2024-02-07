// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2024 Petr Vorel <pvorel@suse.cz>
 */

#include "tst_defaults.h"
#include "tst_test.h"

static void do_test(void)
{
	char cwd[PATH_MAX];

	tst_res(TINFO, "PWD: '%s'", getcwd(cwd, sizeof(cwd)));

	//if (strstr(TEMPDIR, cwd) == NULL)
	char *foo = strstr(TEMPDIR, cwd);
	if (foo == NULL)
		tst_res(TFAIL, "cwd not in %s (%s)", TEMPDIR, cwd);
	else
		tst_res(TPASS, "cwd in %s (%s)", TEMPDIR, cwd);

}

static struct tst_test test = {
	.test_all = do_test,
	.needs_tmpdir = 1,
};
