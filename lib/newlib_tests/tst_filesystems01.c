// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 Wei Gao <wegao@suse.com>
 */

#include "tst_test.h"
#include "tst_safe_stdio.h"

#define MOUNT_POINT "mount_test_filesystems"

static int check_inode_size(unsigned int size)
{
	char path[PATH_MAX];
	char line[PATH_MAX];
	FILE *tune2fs;
	char str_size[NAME_MAX];

	snprintf(str_size, sizeof(str_size), "%u", size);
	snprintf(path, sizeof(path), "tune2fs -l %s 2>&1", tst_device->dev);
	tune2fs = SAFE_POPEN(path, "r");
	if (tune2fs == NULL)
		tst_brk(TBROK, "Can not popen %s", path);

	while (fgets(line, PATH_MAX, tune2fs)) {
		if (*line && strstr(line, "Inode size:") && strstr(line, str_size)) {
			tst_res(TPASS, "check inode size pass");
			return 0;
		}
	}

	pclose(tune2fs);
	tst_res(TFAIL, "check inode size failed");
	return -1;
}

static int check_mnt_data(char *opt)
{
	FILE *fp;
	char line[PATH_MAX];

	fp = SAFE_FOPEN("/proc/mounts", "r");
	if (fp == NULL)
		tst_brk(TBROK, "Can not open /proc/mounts");
	while (fgets(line, PATH_MAX, fp)) {
		if (*line && strstr(line, tst_device->dev) && strstr(line, opt)) {
			tst_res(TPASS, "check mnt data pass");
			return 0;
		}
	}
	SAFE_FCLOSE(fp);
	tst_res(TFAIL, "check mnt data failed");
	return -1;
}

static int check_mkfs_size_opt(unsigned int size)
{
	char path[PATH_MAX];
	char line[PATH_MAX];
	FILE *dumpe2fs;
	char str_size[NAME_MAX];

	snprintf(str_size, sizeof(str_size), "%u", size);
	snprintf(path, sizeof(path), "dumpe2fs -h %s 2>&1", tst_device->dev);
	dumpe2fs = SAFE_POPEN(path, "r");
	if (dumpe2fs == NULL)
		tst_brk(TBROK, "Can not popen %s", path);

	while (fgets(line, PATH_MAX, dumpe2fs)) {
		if (*line && strstr(line, "Block count:") && strstr(line, str_size)) {
			tst_res(TPASS, "check mkfs size opt pass");
			return 0;
		}
	}

	pclose(dumpe2fs);
	tst_res(TFAIL, "check mkfs size opt failed");
	return -1;
}

static void do_test(void)
{
	long fs_type;

	fs_type = tst_fs_type(MOUNT_POINT);

	if (fs_type == TST_EXT234_MAGIC) {
		check_inode_size(128);
		check_mkfs_size_opt(10240);
	}

	if (fs_type == TST_XFS_MAGIC)
		check_mnt_data("usrquota");
}

static struct tst_test test = {
	.test_all = do_test,
	.needs_root = 1,
	.mntpoint = MOUNT_POINT,
	.mount_device = 1,
	.needs_cmds = (const char *[]) {
		"tune2fs",
		"dumpe2fs",
		NULL
	},
	.filesystems = (struct tst_fs []) {
		{
			.type = "ext3",
			.mkfs_opts = (const char *const []){"-I", "128", NULL},
			.mkfs_size_opt = "10240",
		},
		{
			.type = "xfs",
			.mnt_data = "usrquota",
		},
		{}
	},

};
