// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2018 Linux Test Project
 * Copyright (C) 2015 Cyril Hrubis chrubis@suse.cz
 */

#define TST_NO_DEFAULT_MAIN
#include "tst_test.h"

int main(int argc, char *argv[])
{

	tst_res(TPASS, "%s executed", argv[0]);

	return 0;
}
