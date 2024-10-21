// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2013 Oracle and/or its affiliates. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it would be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write the Free Software Foundation,
 * Inc.,  51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Alexey Kodanev <alexey.kodanev@oracle.com>
 *
 * Test checks block device kernel API.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "tst_test.h"
#include "tst_module.h"

#define MODULE_NAME "ltp_block_dev"
#define MODULE_NAME_KO	MODULE_NAME ".ko"

char *TCID = "block_dev";

static const char dev_result[]	= "/sys/devices/ltp_block_dev/result";
static const char dev_tcase[]	= "/sys/devices/ltp_block_dev/tcase";

static int module_loaded;
static char* run_all_testcases;
static struct tst_option options[] = {
	{"a",  &run_all_testcases, "-a\tRun all test-cases (can crash the kernel)"},
	{NULL, NULL, NULL}
};

static void cleanup(void)
{
	if (module_loaded)
		tst_module_unload(MODULE_NAME_KO);
}

static void run(unsigned int n)
{
	/*
	 * test-cases #8 and #9 can crash the kernel.
	 * We have to wait for kernel fix where register_blkdev() &
	 * unregister_blkdev() checks the input device name parameter
	 * against NULL pointer.
	 */
	n++;
	if (!run_all_testcases && (n == 8 || n == 9)) {
		tst_res(TCONF, "Skipped n = %d", n);
		return;
	}

	if (!module_loaded) {
		tst_module_load(MODULE_NAME_KO, NULL);
		module_loaded = 1;
	}

	int pass = 0;
	SAFE_FILE_PRINTF(dev_tcase, "%d", n);
	SAFE_FILE_SCANF(dev_result, "%d", &pass);
	tst_res((pass) ? TPASS : TFAIL, "Test-case '%d'", n);
}

static struct tst_test test = {
	.needs_root = 1,
	.cleanup = cleanup,
	.test = run,
	.tcnt = 9,
	.options = options,
};
