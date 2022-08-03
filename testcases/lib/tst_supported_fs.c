// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2018 FUJITSU LIMITED. All rights reserved.
 * Author: Xiao Yang <yangx.jy@cn.fujitsu.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SKIP_DELIMITER ','

#define TST_NO_DEFAULT_MAIN
#include "tst_test.h"
#include "tst_fs.h"

static void usage(void)
{
	fprintf(stderr, "Usage: tst_supported_fs [-s skip_list] [fs_type]\n");
	fprintf(stderr, "   If fs_type is supported, return 0\n");
	fprintf(stderr, "   If fs_type isn't supported, return 1\n");
	fprintf(stderr, "   If fs_type isn't specified, print the list of supported filesystems\n");
	fprintf(stderr, "   fs_type - a specified filesystem type\n");
	fprintf(stderr, "   skip_list - filesystems to skip, delimiter: '%c'\n",
			SKIP_DELIMITER);
}

int main(int argc, char *argv[])
{
	const char *const *filesystems;
	int i, ret, cnt = 1;
	char **skiplist = NULL;
	char *fs;

	while ((ret = getopt(argc, argv, "hs:"))) {
		if (ret < 0)
			break;

		switch (ret) {
		case '?':
			return 1;

		case 'h':
			usage();
			return 0;

		case 's':
			fs = optarg;
			for (i = 0; fs[i]; i++) {
				if (optarg[i] == SKIP_DELIMITER)
					cnt++;
			}
			skiplist = malloc(++cnt * sizeof(char *));
			if (!skiplist) {
				fprintf(stderr, "malloc() failed\n");
				return 1;
			}

			fs = optarg;
			for (i = 0; i < cnt; i++)
				skiplist[i] = strtok_r(fs, TST_TO_STR(SKIP_DELIMITER), &fs);
			break;
		}
	}

	if (argc - optind > 1) {
		fprintf(stderr, "Can't specify multiple fs_type\n");
		usage();
		return 2;
	}

	if (optind < argc)
		return !tst_fs_is_supported(argv[optind]);

	filesystems = tst_get_supported_fs_types((const char * const*)skiplist);
	for (i = 0; filesystems[i]; i++)
		printf("%s\n", filesystems[i]);

	return 0;
}
