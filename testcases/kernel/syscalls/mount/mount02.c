// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Wipro Technologies Ltd, 2002.  All Rights Reserved.
 *               Nirmala Devi Dhanasekar <nirmala.devi@wipro.com>
 * Copyright (C) 2023 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Check for basic errors returned by mount(2) system call.
 *
 * - ENODEV if filesystem type not configured
 * - ENOTBLK if specialfile is not a block device
 * - EBUSY if specialfile is already mounted or it  cannot  be remounted
 *   read-only, because it still holds files open for writing.
 * - EINVAL if specialfile or device is invalid or a remount was attempted,
 *   while source was not already mounted on target.
 * - EFAULT if special file or device file points to invalid address space.
 * - ENAMETOOLONG if pathname was longer than MAXPATHLEN.
 * - ENOENT if pathname was empty or has a nonexistent component.
 * - ENOTDIR if not a directory.
 */

#include "tst_test.h"
#include <sys/mount.h>
#include <sys/sysmacros.h>

#define MNTPOINT "mntpoint"

static char path[PATH_MAX + 2];
static const char *long_path = path;
static const char *device;
static const char *fs_type;
static const char *null;
static const char *wrong_fs_type = "error";
static const char *mntpoint = MNTPOINT;
static const char *fault = (void *)-1;
static const char *nonexistent = "nonexistent";
static const char *char_dev = "char_device";
static const char *file = "filename";
static int fd;

static void pre_mount(void);
static void post_umount(void);
static void pre_create_file(void);
static void post_delete_file(void);

static struct test_case {
	const char **device;
	const char **mntpoint;
	const char **fs_type;
	const char *fs_str;
	unsigned long flag;
	int exp_errno;
	void (*setup)(void);
	void (*cleanup)(void);
} test_cases[] = {
	{&device, &mntpoint, &wrong_fs_type, "wrong FS type", 0, ENODEV, NULL, NULL},
	{&char_dev, &mntpoint, &fs_type, "char device", 0, ENOTBLK, NULL, NULL},
	{&device, &mntpoint, &fs_type, "mounted folder", 0, EBUSY, pre_mount, post_umount},
	{&device, &mntpoint, &fs_type, "mounted folder containing file", MS_REMOUNT | MS_RDONLY, EBUSY, pre_create_file, post_delete_file},
	{&null, &mntpoint, &fs_type, "invalid device", 0, EINVAL, NULL, NULL},
	{&device, &mntpoint, &null, "invalid device type", 0, EINVAL, NULL, NULL},
	{&device, &mntpoint, &fs_type, "mounted folder", MS_REMOUNT, EINVAL, NULL, NULL},
	{&fault, &mntpoint, &fs_type, "fault device", 0, EFAULT, NULL, NULL},
	{&device, &mntpoint, &fault, "fault device type", 0, EFAULT, NULL, NULL},
	{&device, &long_path, &fs_type, "long name", 0, ENAMETOOLONG, NULL, NULL},
	{&device, &nonexistent, &fs_type, "non existant folder", 0, ENOENT, NULL, NULL},
	{&device, &file, &fs_type, "file", 0, ENOTDIR, NULL, NULL},
};

static void pre_mount(void)
{
	SAFE_MOUNT(device, mntpoint, fs_type, 0, NULL);
}

static void post_umount(void)
{
	if (tst_is_mounted(MNTPOINT))
		SAFE_UMOUNT(MNTPOINT);
}

static void pre_create_file(void)
{
	pre_mount();
	fd = SAFE_OPEN("mntpoint/file", O_CREAT | O_RDWR, 0700);
}

static void post_delete_file(void)
{
	SAFE_CLOSE(fd);
	post_umount();
}

static void setup(void)
{
	device = tst_device->dev;
	fs_type = tst_device->fs_type;

	memset(path, 'a', PATH_MAX + 1);

	SAFE_MKNOD(char_dev, S_IFCHR | 0777, 0);
	SAFE_TOUCH(file, 0777, 0);
}

static void cleanup(void)
{
	if (tst_is_mounted(MNTPOINT))
		SAFE_UMOUNT(MNTPOINT);
}

static void run(unsigned int i)
{
	struct test_case *tc = &test_cases[i];

	if (tc->setup)
		tc->setup();

	TST_EXP_FAIL(mount(*tc->device, *tc->mntpoint, *tc->fs_type, tc->flag, NULL),
		tc->exp_errno,
		"mounting %s",
		tc->fs_str);

	if (tc->cleanup)
		tc->cleanup();
}

static struct tst_test test = {
	.tcnt = ARRAY_SIZE(test_cases),
	.test = run,
	.setup = setup,
	.cleanup = cleanup,
	.needs_root = 1,
	.needs_tmpdir = 1,
	.format_device = 1,
	.mntpoint = MNTPOINT,
};
