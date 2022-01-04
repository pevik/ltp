// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright (c) 2021 FUJITSU LIMITED. All rights reserved.*/

#include <stdio.h>
#include "tst_kconfig.h"

int main(int argc, const char *argv[])
{
	if (argc < 2) {
		fprintf(stderr, "Please provide kernel kconfig list\n");
		return 1;
	}

	if (tst_kconfig_check(argv+1))
		return 1;
	return 0;
}
