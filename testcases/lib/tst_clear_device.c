// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2022 Petr Vorel <pvorel@suse.cz>
 * Copyright (c) 2017 Cyril Hrubis <chrubis@suse.cz>
 */

#include <stdio.h>
#include <sys/stat.h>
#define TST_NO_DEFAULT_MAIN
#include "tst_test.h"
#include "old/old_device.h"

extern struct tst_test *tst_test;

static struct tst_test test = {
};

static void print_help(void)
{
	fprintf(stderr, "\nUsage: tst_clear_device block_device\n");
}

int main(int argc, char *argv[])
{
	/*
	 * Force messages to be printed from the new library i.e. tst_test.c
	 *
	 * The new library prints messages into stderr while the old one prints
	 * them into stdout. When messages are printed into stderr we can
	 * safely do:
	 *
	 * DEV=$(tst_device acquire)
	 */
	tst_test = &test;
	struct stat st;

	if (argc < 2)
		goto help;

	if (stat(argv[1], &st) < 0 || !S_ISBLK(st.st_mode))
		goto help;

	return tst_clear_device(argv[1]);
help:
	print_help();
	return 1;
}
