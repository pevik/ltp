// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) Linux Test Project, 2022
 * Copyright (c) Wipro Technologies Ltd, 2002.  All Rights Reserved.
 */

/*\
 * [Description]
 *
 * Verify mount(2) for various flags.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>
#include "tst_test.h"
#include "lapi/mount.h"

#define MNTPOINT	"mntpoint"
#define TESTBIN	"mount03_setuid_test"
#define BIN_PATH	MNTPOINT "/" TESTBIN
#define TEST_STR "abcdefghijklmnopqrstuvwxyz"
#define FILE_MODE	0644
#define SUID_MODE	0511

static int otfd;
static char wbuf[BUFSIZ];
static char rbuf[BUFSIZ];
static char file[PATH_MAX];
static uid_t nobody_uid;
static gid_t nobody_gid;

static void test_rdonly(void)
{
	snprintf(file, PATH_MAX, "%s/rdonly", MNTPOINT);
	TST_EXP_FAIL(otfd = open(file, O_CREAT | O_RDWR, 0700), EROFS);
}

static void test_nodev(void)
{
	snprintf(file, PATH_MAX, "%s/nodev", MNTPOINT);
	SAFE_MKNOD(file, S_IFBLK | 0777, 0);
	TST_EXP_FAIL(otfd = open(file, O_RDWR, 0700), EACCES);
	SAFE_UNLINK(file);
}

static void test_noexec(void)
{
	snprintf(file, PATH_MAX, "%s/noexec", MNTPOINT);
	otfd = SAFE_OPEN(file, O_CREAT | O_RDWR, 0700);
	TST_EXP_FAIL(execlp(file, basename(file), NULL), EACCES);
}

static void test_synchronous(void)
{
	strcpy(wbuf, TEST_STR);
	snprintf(file, PATH_MAX, "%s/synchronous", MNTPOINT);
	otfd = SAFE_OPEN(file, O_RDWR | O_CREAT, FILE_MODE);
	SAFE_WRITE(1, otfd, wbuf, strlen(wbuf));
	SAFE_LSEEK(otfd, 0, SEEK_SET);
	SAFE_READ(0, otfd, rbuf, sizeof(rbuf));
	TST_EXP_EQ_STR(rbuf, wbuf);
}

static void test_remount(void)
{
	SAFE_MOUNT(tst_device->dev, MNTPOINT, tst_device->fs_type, MS_REMOUNT, NULL);
	snprintf(file, PATH_MAX, "%s/remount", MNTPOINT);
	TST_EXP_FD(otfd = open(file, O_CREAT | O_RDWR, 0700));
}

static void test_nosuid(void)
{
	pid_t pid;
	int status;
	struct stat st;

	SAFE_CP(TESTBIN, BIN_PATH);
	SAFE_STAT(BIN_PATH, &st);
	if (st.st_mode != SUID_MODE)
		SAFE_CHMOD(BIN_PATH, SUID_MODE);

	pid = SAFE_FORK();
	if (!pid) {
		SAFE_SETGID(nobody_gid);
		SAFE_SETREUID(-1, nobody_uid);
		SAFE_EXECL(BIN_PATH, TESTBIN, NULL);
	}

	SAFE_WAITPID(pid, &status, 0);

	if (WIFEXITED(status)) {
		switch (WEXITSTATUS(status)) {
		case EXIT_FAILURE:
			tst_res(TFAIL, "%s failed", TESTBIN);
			return;
		case EXIT_SUCCESS:
			tst_res(TPASS, "%s passed", TESTBIN);
			return;
		default:
		case TBROK:
			break;
		}
	}

	tst_brk(TBROK, "Child %s", tst_strstatus(status));
}

static void test_noatime(void)
{
	time_t atime;
	struct stat st;
	char readbuf[20];

	snprintf(file, PATH_MAX, "%s/noatime", MNTPOINT);
	TST_EXP_FD_SILENT(otfd = open(file, O_CREAT | O_RDWR, 0700));

	SAFE_WRITE(1, otfd, TEST_STR, strlen(TEST_STR));
	SAFE_FSTAT(otfd, &st);
	atime = st.st_atime;
	sleep(1);

	SAFE_READ(0, otfd, readbuf, sizeof(readbuf));
	SAFE_FSTAT(otfd, &st);
	TST_EXP_EQ_LI(st.st_atime, atime);
}

#define FLAG_DESC(x) .flag = x, .desc = #x
static struct tcase {
	unsigned int flag;
	char *desc;
	void (*test)(void);
} tcases[] = {
	{FLAG_DESC(MS_RDONLY), test_rdonly},
	{FLAG_DESC(MS_NODEV), test_nodev},
	{FLAG_DESC(MS_NOEXEC), test_noexec},
	{FLAG_DESC(MS_SYNCHRONOUS), test_synchronous},
	{FLAG_DESC(MS_RDONLY), test_remount},
	{FLAG_DESC(MS_NOSUID), test_nosuid},
	{FLAG_DESC(MS_NOATIME), test_noatime},
};

static void setup(void)
{
	struct passwd *ltpuser = SAFE_GETPWNAM("nobody");

	nobody_uid = ltpuser->pw_uid;
	nobody_gid = ltpuser->pw_gid;
}

static void cleanup(void)
{
	if (otfd >= 0)
		SAFE_CLOSE(otfd);

	if (tst_is_mounted(MNTPOINT))
		SAFE_UMOUNT(MNTPOINT);
}


static void run(unsigned int n)
{
	struct tcase *tc = &tcases[n];

	tst_res(TINFO, "Testing flag %s", tc->desc);

	TST_EXP_PASS_SILENT(mount(tst_device->dev, MNTPOINT, tst_device->fs_type,
		   tc->flag, NULL));
	if (!TST_PASS)
		return;

	if (tc->test)
		tc->test();

	cleanup();
}

static struct tst_test test = {
	.tcnt = ARRAY_SIZE(tcases),
	.test = run,
	.setup = setup,
	.cleanup = cleanup,
	.needs_root = 1,
	.format_device = 1,
	.resource_files = (const char *const[]) {
		TESTBIN,
		NULL,
	},
	.forks_child = 1,
	.mntpoint = MNTPOINT,
	.all_filesystems = 1,
	.skip_filesystems = (const char *const []){
		"exfat",
		"vfat",
		"ntfs",
		NULL
	},
};
