// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2017 Cyril Hrubis <chrubis@suse.cz>
 */

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#define TST_NO_DEFAULT_MAIN
#include "tst_test.h"
#include "old/old_device.h"

extern struct tst_test *tst_test;

static struct tst_test test = {
};

static void print_help(void)
{
	fprintf(stderr, "\nUsage:\n");
	fprintf(stderr, "tst_device [-s size [-d /path/to/device]] acquire\n");
	fprintf(stderr, "tst_device [-d /path/to/device] clear\n");
	fprintf(stderr, "tst_device -d /path/to/device release\n");
	fprintf(stderr, "tst_device -h\n\n");
}

static int acquire_device(const char *device_path, unsigned int size)
{
	const char *device;

	if (device_path)
		device = tst_acquire_loop_device(size, device_path);
	else
		device = tst_acquire_device__(size, TST_ALL_FILESYSTEMS);

	if (!device)
		return 1;

	if (tst_clear_device(device)) {
		tst_release_device(device);
		return 1;
	}

	printf("%s", device);

	return 0;
}

static int release_device(const char *device_path)
{
	if (!device_path) {
		fprintf(stderr, "ERROR: Missing /path/to/device\n");
		return 1;
	}

	/*
	 * tst_acquire_[loop_]device() was called in a different process.
	 * tst_release_device() would think that no device was acquired yet
	 * and do nothing. Call tst_detach_device() directly to bypass
	 * the check.
	 */
	return tst_detach_device(device_path);
}

static int clear_device(int argc, char *argv[])
{
	if (argc != 3)
		return 1;

	if (tst_clear_device(argv[2]))
		return 1;

	return 0;
}

int main(int argc, char *argv[])
{
	char *device_path = NULL;
	unsigned int size = 0;
	int ret;

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

	while ((ret = getopt(argc, argv, "d:hs:"))) {
		if (ret < 0)
			break;

		switch (ret) {
		case 'd':
			device_path = optarg;
			break;
		case 'h':
			print_help();
			return 0;
		case 's':
			size = atoi(optarg);
			if (!size) {
				fprintf(stderr, "ERROR: Invalid device size '%s'", optarg);
				return 1;
			}
			break;
		}
	}

	if (argc - optind < 1)
		goto help;

	if (!strcmp(argv[optind], "acquire")) {
		if (acquire_device(device_path, size))
			goto help;
	} else if (!strcmp(argv[optind], "release")) {
		if (release_device(device_path))
			goto help;
	} else if (!strcmp(argv[1], "clear")) {
		if (clear_device(argc, argv))
			goto help;
	} else {
		fprintf(stderr, "ERROR: Invalid COMMAND '%s'\n", argv[optind]);
		goto help;
	}

	return 0;
help:
	print_help();
	return 1;
}
