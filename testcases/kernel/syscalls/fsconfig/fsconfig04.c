// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 Wei Gao <wegao@suse.com>
 */

/*\
 * This test aims to validate the functionality of fsconfig(FSCONFIG_SET_PATH) in
 * dynamically altering the external journal device of a ext3 or ext4 filesystem.
 * Case acquires three loop devices (dev0, dev1, dev2), it formats dev1 and dev2
 * as external journal devices using the -O journal_dev option and assigns them
 * the same UUID. Then formats dev0 (the main filesystem) multiple times, first
 * associating it with dev1, then change to dev2, finally back to dev1 again as
 * an external journal using the -J device= option.
 *
 * 2 workarounds in this case need mention:
 *
 * - To avoid "journal UUID does not match" error when switch external journal device
 *   we have to assign same UUID to dev1/dev2
 * - Before fsconfig test we have to formats dev0 associating to dev1->dev2->dev1,
 *   this will make sure both dev1/2's supper block contain correct content. Otherwise
 *   you will encounter error such as "EXT4-fs (loop0): External journal has more than
 *   one user (unsupported) - 0" when switch external journal device using fsconfig.
 */

#include "tst_test.h"
#include "lapi/fsmount.h"
#include "old/old_device.h"

#define MNTPOINT	"mntpoint"
#define LOOP_DEV_SIZE 10
#define UUID "d73c9e5e-97e4-4a9c-b17e-75a931b02660"

static int fd;
static const char *dev;
static char dev0[NAME_MAX];
static char dev1[NAME_MAX];
static char dev2[NAME_MAX];

static void cleanup(void)
{
	if (fd != -1)
		SAFE_CLOSE(fd);

	TST_EXP_PASS_SILENT(tst_detach_device(dev1));
	TST_EXP_PASS_SILENT(tst_detach_device(dev2));
}

static void setup(void)
{
	strcpy(dev0, tst_device->dev);
	dev = tst_acquire_loop_device(LOOP_DEV_SIZE, "dev1_file");
	if (!dev)
		tst_brk(TBROK, "acquire loop dev1 failed");

	strcpy(dev1, dev);
	dev = NULL;

	dev = tst_acquire_loop_device(LOOP_DEV_SIZE, "dev2_file");
	if (!dev)
		tst_brk(TBROK, "acquire loop dev2 failed");
	strcpy(dev2, dev);

	const char *const *mkfs_opts_set_UUID;
	const char *const *mkfs_opts_set_journal_dev1;
	const char *const *mkfs_opts_set_journal_dev2;

	mkfs_opts_set_UUID = (const char *const []) {"-F", "-U", UUID,
		"-O journal_dev", NULL};

	char device_option_dev1[PATH_MAX];
	char device_option_dev2[PATH_MAX];

	sprintf(device_option_dev1, "device=%s", dev1);
	sprintf(device_option_dev2, "device=%s", dev2);

	mkfs_opts_set_journal_dev1 = (const char *const []) {"-F", "-J",
		device_option_dev1, NULL};

	mkfs_opts_set_journal_dev2 = (const char *const []) {"-F", "-J",
		device_option_dev2, NULL};

	SAFE_MKFS(dev1, tst_device->fs_type, mkfs_opts_set_UUID, NULL);
	SAFE_MKFS(dev2, tst_device->fs_type, mkfs_opts_set_UUID, NULL);
	SAFE_MKFS(dev0, tst_device->fs_type, mkfs_opts_set_journal_dev1, NULL);
	SAFE_MKFS(dev0, tst_device->fs_type, mkfs_opts_set_journal_dev2, NULL);
	SAFE_MKFS(dev0, tst_device->fs_type, mkfs_opts_set_journal_dev1, NULL);
}

static void run(void)
{
	TEST(fd = fsopen(tst_device->fs_type, 0));
	if (fd == -1)
		tst_brk(TBROK | TTERRNO, "fsopen() failed");

	TEST(fsconfig(fd, FSCONFIG_SET_STRING, "source", dev0, 0));
	if (TST_RET == -1)
		tst_brk(TFAIL | TTERRNO, "fsconfig(FSCONFIG_SET_STRING) failed");

	TEST(fsconfig(fd, FSCONFIG_SET_PATH, "journal_path", dev2, 0));
	if (TST_RET == -1) {
		if (TST_ERR == EOPNOTSUPP)
			tst_brk(TCONF, "fsconfig(FSCONFIG_SET_PATH) not supported");
		else
			tst_brk(TFAIL | TTERRNO, "fsconfig(FSCONFIG_SET_PATH) failed");
	}

	TEST(fsconfig(fd, FSCONFIG_CMD_CREATE, NULL, NULL, 0));
	if (TST_RET == -1)
		tst_brk(TFAIL | TTERRNO, "fsconfig(FSCONFIG_CMD_CREATE) failed");

	char loop_name[NAME_MAX];
	char path[PATH_MAX];
	char device_str[NAME_MAX];
	unsigned int major, minor, device_num;
	unsigned int found = 0;

	SAFE_SSCANF(dev2, "/dev/%s", loop_name);

	sprintf(path, "/sys/block/%s/dev", loop_name);
	SAFE_FILE_SCANF(path, "%u:%u", &major, &minor);
	device_num = (major << 8) | minor;
	sprintf(device_str, "0x%04X", device_num);

	char line[PATH_MAX];
	FILE *tune2fs;

	sprintf(path, "tune2fs -l %s 2>&1", dev0);
	tune2fs = popen(path, "r");

	while (fgets(line, PATH_MAX, tune2fs)) {
		if (*line && strstr(line, "Journal device:") && strstr(line, device_str)) {
			found = 1;
			break;
		}
	}

	if (found == 1)
		tst_res(TPASS, "Device found in journal");
	else
		tst_res(TFAIL, "Device not found in journal");

	pclose(tune2fs);
	SAFE_CLOSE(fd);
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.cleanup = cleanup,
	.needs_root = 1,
	.format_device = 1,
	.mntpoint = MNTPOINT,
	.all_filesystems = 1,
	.needs_cmds = (const char *[]) {
		"tune2fs",
		NULL
	},
	.skip_filesystems = (const char *const []){"ntfs", "vfat", "exfat",
		"ext2", "tmpfs", "xfs", "btrfs", NULL},
};
