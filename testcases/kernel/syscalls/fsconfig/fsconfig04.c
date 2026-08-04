// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2026 Wei Gao <wegao@suse.com>
 */

/*\
 * This test aims to validate :manpage:`fsconfig(2)` with the
 * ``FSCONFIG_SET_PATH`` operation in dynamically altering the external
 * journal device of an ext3 or ext4 filesystem.
 *
 * Root privileges are required because the test creates and formats loop
 * devices, and configures filesystems.
 *
 * [Algorithm]
 *
 * - Acquire three loop devices (``dev0`` from framework, ``dev1``, ``dev2``).
 * - Format ``dev1`` and ``dev2`` as external journal devices with the same UUID
 *   (``-O journal_dev -U <uuid>``).
 * - Format ``dev0`` three times to cycle journal associations:
 *   ``dev1`` -> ``dev2`` -> ``dev1``, so both journal superblocks are consistent.
 * - Open ``dev0`` via :manpage:`fsopen(2)` and set the source and journal_path using
 *   :manpage:`fsconfig(2)` (``FSCONFIG_SET_STRING``) and
 *   :manpage:`fsconfig(2)` (``FSCONFIG_SET_PATH``).
 * - Apply ``FSCONFIG_CMD_CREATE`` and verify with ``tune2fs`` that ``dev0``'s
 *   journal device is now ``dev2``.
 *
 * Implementation notes:
 *
 * - To avoid ``journal UUID does not match`` error when switching external
 *   journal device, we have to assign the same UUID to ``dev1``/``dev2``.
 * - Before the :manpage:`fsconfig(2)` test, we have to format ``dev0`` associating to
 *   ``dev1`` -> ``dev2`` -> ``dev1``. This ensures that both ``dev1``/``dev2`` superblocks contain
 *   correct content. Otherwise, you will encounter errors such as
 *   ``EXT4-fs (loop0): External journal has more than one user (unsupported)``
 *   when switching the external journal device using :manpage:`fsconfig(2)`.
 */

#include "tst_test.h"
#include "tst_safe_stdio.h"
#include "lapi/fsmount.h"

#define MNTPOINT	"mntpoint"
#define LOOP_DEV_SIZE 10
#define UUID "d73c9e5e-97e4-4a9c-b17e-75a931b02660"

static int fd = -1;
static char dev0[PATH_MAX];
static char dev1[PATH_MAX];
static char dev2[PATH_MAX];

static void cleanup(void)
{
	if (fd != -1)
		SAFE_CLOSE(fd);

	if (dev1[0])
		tst_detach_device(dev1);

	if (dev2[0])
		tst_detach_device(dev2);
}

static void create_and_attach_loopdev(const char *filename, char *dev_path, size_t dev_path_len)
{
	if (tst_prealloc_file(filename, 1024 * 1024, LOOP_DEV_SIZE))
		tst_brk(TBROK, "Failed to create %s", filename);

	if (tst_find_free_loopdev(dev_path, dev_path_len) == -1)
		tst_brk(TBROK, "No free loop device found for %s", filename);

	if (tst_attach_device(dev_path, filename))
		tst_brk(TBROK, "Failed to attach %s to %s", filename, dev_path);
}

static void setup(void)
{
	fsopen_supported_by_kernel();

	strcpy(dev0, tst_device->dev);

	create_and_attach_loopdev("dev1_file", dev1, sizeof(dev1));
	create_and_attach_loopdev("dev2_file", dev2, sizeof(dev2));

	const char *const *mkfs_opts_set_UUID;
	const char *const *mkfs_opts_set_journal_dev1;
	const char *const *mkfs_opts_set_journal_dev2;

	mkfs_opts_set_UUID = (const char *const []) {"-F", "-U", UUID,
		"-O", "journal_dev", NULL};

	char device_option_dev1[PATH_MAX + 16];
	char device_option_dev2[PATH_MAX + 16];

	snprintf(device_option_dev1, sizeof(device_option_dev1), "device=%s", dev1);
	snprintf(device_option_dev2, sizeof(device_option_dev2), "device=%s", dev2);

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
		tst_brk(TBROK | TTERRNO, "fsconfig(FSCONFIG_SET_STRING) failed");

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
	char path[PATH_MAX + 32];
	char device_str[NAME_MAX];
	unsigned int major, minor, device_num;
	unsigned int found = 0;

	SAFE_SSCANF(dev2, "/dev/%s", loop_name);

	snprintf(path, sizeof(path), "/sys/block/%s/dev", loop_name);
	SAFE_FILE_SCANF(path, "%u:%u", &major, &minor);
	device_num = (minor & 0xff) | (major << 8) | ((minor & ~0xff) << 12);
	snprintf(device_str, sizeof(device_str), "0x%04x", device_num);

	char line[PATH_MAX];
	FILE *tune2fs;

	snprintf(path, sizeof(path), "tune2fs -l %s 2>&1", dev0);
	tune2fs = SAFE_POPEN(path, "r");

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
	.needs_tmpdir = 1,
	.needs_device = 1,
	.mntpoint = MNTPOINT,
	.needs_cmds = (struct tst_cmd []) {
		{.cmd = "tune2fs"},
		{}
	},
	.filesystems = (struct tst_fs []) {
		{.type = "ext3"},
		{.type = "ext4"},
		{}
	},
};
