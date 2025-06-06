// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines  Corp., 2001
 * 07/2001 ported by John George
 * Copyright (c) 2022 SUSE LLC Avinesh Kumar <avinesh.kumar@suse.com>
 */

/*\
 * Verify that the system call utime() successfully changes the last
 * access and modification times of a file to the values specified by
 * times argument, under the following constraints:
 *
 * - The times argument is not NULL.
 * - The user ID of the process is "root".
 */

#include <utime.h>
#include "tst_test.h"

#define MNT_POINT	"mntpoint"
#define TEMP_FILE	MNT_POINT"/tmp_file"

#define FILE_MODE	0444
#define NEW_MODF_TIME	10000
#define NEW_ACCESS_TIME	20000

static struct utimbuf times = {
	.modtime = NEW_MODF_TIME,
	.actime = NEW_ACCESS_TIME
};

static void setup(void)
{
	SAFE_TOUCH(TEMP_FILE, FILE_MODE, NULL);
}

static void run(void)
{
	struct stat stat_buf;

	TST_EXP_PASS(utime(TEMP_FILE, &times), "utime(%s, &times)", TEMP_FILE);
	if (!TST_PASS)
		return;

	SAFE_STAT(TEMP_FILE, &stat_buf);

	TST_EXP_EQ_LI(stat_buf.st_mtime, NEW_MODF_TIME);
	TST_EXP_EQ_LI(stat_buf.st_atime, NEW_ACCESS_TIME);
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.needs_root = 1,
	.mount_device = 1,
	.mntpoint = MNT_POINT,
	.all_filesystems = 1,
	.skip_filesystems = (const char *const[]) {
		"vfat",
		"exfat",
		NULL
	}
};
