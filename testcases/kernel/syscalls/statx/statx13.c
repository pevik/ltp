// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2026 SUSE LLC <gaurav.pathak@suse.com>
 */

/*\
 * This test validates the STATX_WRITE_ATOMIC feature (introduced in Linux 6.13).
 * It ensures that supported filesystems (xfs as of now) correctly report their
 * atomic write limits to user space when queried via statx().
 *
 * The test performs the following validations:
 * - Creates a test file using O_DIRECT (a prerequisite for atomic writes).
 * - Calls statx() with the STATX_WRITE_ATOMIC mask to retrieve the limits.
 * - Verifies that stx_atomic_write_unit_min, stx_atomic_write_unit_max, and
 *    stx_atomic_write_unit_max_opt are logically consistent (e.g., max_opt is
 *    within the min and max bounds).
 * - Ensures all reported atomic write unit sizes are valid powers of two.
 */

#define _GNU_SOURCE
#include <sys/param.h>
#include "tst_test.h"

#define MNTPOINT "mnt_point"
#define TESTFILE MNTPOINT"/testfile"
#define MODE 0644

#define WRITE_SIZE 4096
#define ALIGNMENT  4096

static int file_fd = -1;

static void verify_statx(void)
{
	struct statx buff;

	TST_EXP_PASS_SILENT(statx(AT_FDCWD, TESTFILE, 0, STATX_BASIC_STATS | STATX_WRITE_ATOMIC, &buff),
			"statx(AT_FDCWD, %s, 0, STATX_WRITE_ATOMIC, &buf)", TESTFILE);

	if (!(buff.stx_attributes & STATX_ATTR_WRITE_ATOMIC)) {
		tst_res(TCONF, "Filesystem does not support STATX_WRITE_ATOMIC");
		return;
	}

	if (buff.stx_atomic_write_unit_min > 0 &&
			__builtin_popcount(buff.stx_atomic_write_unit_min) == 1)
		tst_res(TPASS, "stx_atomic_write_unit_min(%u) is power of 2",
				buff.stx_atomic_write_unit_min);
	else
		tst_res(TFAIL, "stx_atomic_write_unit_min(%u) is not a power of 2",
				buff.stx_atomic_write_unit_min);

	if (buff.stx_atomic_write_unit_max > 0 &&
			__builtin_popcount(buff.stx_atomic_write_unit_max) == 1)
		tst_res(TPASS, "stx_atomic_write_unit_max(%u) is power of 2",
				buff.stx_atomic_write_unit_max);
	else
		tst_res(TFAIL, "stx_atomic_write_unit_max(%u) is not a power of 2",
				buff.stx_atomic_write_unit_max);

#ifdef HAVE_STRUCT_STATX_STX_ATOMIC_WRITE_UNIT_MAX_OPT
	if (buff.stx_atomic_write_unit_max_opt == 0) {
		tst_res(TINFO, "stx_atomic_write_unit_max_opt is 0 (no optimized max reported)");
	} else {
		if (buff.stx_atomic_write_unit_max_opt > buff.stx_atomic_write_unit_max)
			tst_res(TFAIL, "stx_atomic_write_unit_max_opt (%u) exceeds max (%u)",
					buff.stx_atomic_write_unit_max_opt,
					buff.stx_atomic_write_unit_max);

		else if (buff.stx_atomic_write_unit_max_opt < buff.stx_atomic_write_unit_min)
			tst_res(TFAIL, "stx_atomic_write_unit_max_opt (%u) is less than min (%u)",
					buff.stx_atomic_write_unit_max_opt,
					buff.stx_atomic_write_unit_min);
		else
			tst_res(TPASS, "stx_atomic_write_unit_max_opt (%u) is within valid range [%u, %u]",
					buff.stx_atomic_write_unit_max_opt,
					buff.stx_atomic_write_unit_min,
					buff.stx_atomic_write_unit_max);

		if (__builtin_popcount(buff.stx_atomic_write_unit_max_opt) != 1)
			tst_res(TFAIL, "stx_atomic_write_unit_max_opt (%u) is not a power of 2",
					buff.stx_atomic_write_unit_max_opt);
	}
#else
	tst_res(TCONF, "stx_atomic_write_unit_max_opt is not defined in struct statx");
#endif
}

static void setup(void)
{
	char *data_buff = SAFE_MEMALIGN(ALIGNMENT, WRITE_SIZE);

	if (strcmp(tst_device->fs_type, "xfs") && strcmp(tst_device->fs_type, "ext4"))
		tst_brk(TCONF, "This test only supports ext4 and xfs");

	umask(0);
	memset(data_buff, '@', WRITE_SIZE);

	file_fd =  SAFE_OPEN(TESTFILE, O_RDWR | O_CREAT | O_DIRECT, MODE);
	SAFE_WRITE(SAFE_WRITE_ALL, file_fd, data_buff, WRITE_SIZE);
}

static void cleanup(void)
{
	if (file_fd > -1)
		SAFE_CLOSE(file_fd);
}

static struct tst_test test = {
	.test_all = verify_statx,
	.setup = setup,
	.cleanup = cleanup,
	.min_kver = "6.13",
	.needs_root = 1,
	.needs_device = 1,
	.needs_tmpdir = 1,
	.mntpoint = MNTPOINT,
	.mount_device = 1,
	.filesystems = (struct tst_fs[]) {
		{
			.type = "xfs",
			.mkfs_opts = (const char *const []){"-f", "-bsize=16K", NULL},
		},
		{
			.type = "ext4",
			.mkfs_opts = (const char *const []){"-O", "bigalloc", "-b", "4096", "-C", "65536", NULL},
		},
		{}
	},
};
