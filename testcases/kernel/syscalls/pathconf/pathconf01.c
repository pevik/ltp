// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2000 Silicon Graphics, Inc.  All Rights Reserved.
 * Copyright (c) Linux Test Project, 2000-2023
 * Authors: William Roske, Dave Fenner
 */

/*\
 * [Description]
 *
 * Check the basic functionality of the pathconf() system call.
 */

#include <stdlib.h>
#include "tst_test.h"

static char *path;

static struct tcase {
	char *name;
	int value;
} tcases[] = {
	{"_PC_LINK_MAX", _PC_LINK_MAX},
	{"_PC_NAME_MAX", _PC_NAME_MAX},
	{"_PC_PATH_MAX", _PC_PATH_MAX},
	{"_PC_PIPE_BUF", _PC_PIPE_BUF},
	{"_PC_CHOWN_RESTRICTED", _PC_CHOWN_RESTRICTED},
	{"_PC_NO_TRUNC", _PC_NO_TRUNC},
	{NULL, 0},
};

static void verify_pathconf(unsigned int i)
{
	struct tcase *tc = &tcases[i];

	path = tst_get_tmpdir();

	TST_EXP_POSITIVE(pathconf(path, tc->value),
			 "pathconf(%s, %s)", path, tc->name);
}

void cleanup(void)
{
	free(path);
}

static struct tst_test test = {
	.needs_tmpdir = 1,
	.test = verify_pathconf,
	.tcnt = ARRAY_SIZE(tcases),
	.cleanup = cleanup,
};
