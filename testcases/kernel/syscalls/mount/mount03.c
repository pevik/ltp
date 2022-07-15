// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) Linux Test Project, 2022
 * Copyright (c) Wipro Technologies Ltd, 2002.  All Rights Reserved.
 */

/*\
 * [Description]
 *
 * Check for basic mount(2) system call flags.
 *
 * Verify that mount(2) syscall passes for each flag setting and validate
 * the flags
 *
 * - MS_RDONLY - mount read-only.
 * - MS_NODEV - disallow access to device special files.
 * - MS_NOEXEC - disallow program execution.
 * - MS_SYNCHRONOUS - writes are synced at once.
 * - MS_REMOUNT - alter flags of a mounted FS.
 * - MS_NOSUID - ignore suid and sgid bits.
 * - MS_NOATIME - do not update access times.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#define TEMP_FILE	"temp_file"
#define FILE_MODE	(S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)
#define SUID_MODE	(S_ISUID|S_IRUSR|S_IXUSR|S_IXGRP|S_IXOTH)

#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <linux/limits.h>
#include <stdlib.h>
#include <pwd.h>
#include "tst_test.h"
#include "tst_safe_file_ops.h"
#include "lapi/mount.h"

#define MNTPOINT        "mntpoint"
#define FILE_PATH	MNTPOINT"/mount03_setuid_test"
#define TCASE_ENTRY(_flags)	{.name = "Flag " #_flags, .flags = _flags}

static int otfd;
static char write_buffer[BUFSIZ];
static char read_buffer[BUFSIZ];
static char file[PATH_MAX];
static struct passwd *ltpuser;

static struct tcase {
	char *name;
	unsigned int flags;
} tcases[] = {
	TCASE_ENTRY(MS_RDONLY),
	TCASE_ENTRY(MS_NODEV),
	TCASE_ENTRY(MS_NOEXEC),
	TCASE_ENTRY(MS_SYNCHRONOUS),
	TCASE_ENTRY(MS_RDONLY),
	TCASE_ENTRY(MS_NOSUID),
	TCASE_ENTRY(MS_NOATIME),
};

static int test_ms_nosuid(void)
{
	int status;
	pid_t pid;

	pid = SAFE_FORK();

	if (!pid) {
		SAFE_SETREUID(ltpuser->pw_uid, ltpuser->pw_uid);
		SAFE_EXECLP(file, basename(file), NULL);
	}

	SAFE_WAITPID(pid, &status, 0);

	return 0;
}

static void test_rwflag(int i)
{
	int ret;
	time_t atime;
	struct stat file_stat;
	char readbuf[20];

	switch (i) {
	case 0:
		/* Validate MS_RDONLY flag of mount call */

		snprintf(file, PATH_MAX, "%s/tmp", MNTPOINT);
		TST_EXP_FAIL2(open(file, O_CREAT | O_RDWR, S_IRWXU),
			      EROFS, "mount(2) passed with flag MS_RDONLY: "
			      "open fail with EROFS as expected");

		otfd = TST_RET;
		break;
	case 1:
		/* Validate MS_NODEV flag of mount call */

		snprintf(file, PATH_MAX, "%s/mynod_%d", MNTPOINT, getpid());
		if (SAFE_MKNOD(file, S_IFBLK | 0777, 0) == 0) {
			TST_EXP_FAIL2(open(file, O_RDWR, S_IRWXU),
				      EACCES, "mount(2) passed with flag MS_NODEV: "
				      "open fail with EACCES as expected");
			otfd = TST_RET;
		}
		SAFE_UNLINK(file);
		break;
	case 2:
		/* Validate MS_NOEXEC flag of mount call */

		snprintf(file, PATH_MAX, "%s/tmp1", MNTPOINT);
		TST_EXP_FD_SILENT(open(file, O_CREAT | O_RDWR, S_IRWXU),
				  "opening %s failed", file);
		otfd = TST_RET;
		if (otfd >= 0) {
			SAFE_CLOSE(otfd);
			ret = execlp(file, basename(file), NULL);
			if ((ret == -1) && (errno == EACCES)) {
				tst_res(TPASS, "mount(2) passed with flag MS_NOEXEC");
				break;
			}
		}
		tst_brk(TFAIL | TERRNO, "mount(2) failed with flag MS_NOEXEC");
		break;
	case 3:
		/*
		 * Validate MS_SYNCHRONOUS flag of mount call.
		 * Copy some data into data buffer.
		 */

		strcpy(write_buffer, "abcdefghijklmnopqrstuvwxyz");

		/* Creat a temporary file under above directory */
		snprintf(file, PATH_MAX, "%s/%s", MNTPOINT, TEMP_FILE);
		TST_EXP_FD_SILENT(open(file, O_RDWR | O_CREAT, FILE_MODE),
			   "open(%s, O_RDWR|O_CREAT, %#o) failed",
			   file, FILE_MODE);
		otfd = TST_RET;

		/* Write the buffer data into file */
		SAFE_WRITE(1, otfd, write_buffer, strlen(write_buffer));

		/* Set the file ptr to b'nning of file */
		SAFE_LSEEK(otfd, 0, SEEK_SET);

		/* Read the contents of file */
		if (SAFE_READ(0, otfd, read_buffer, sizeof(read_buffer)) > 0) {
			if (strcmp(read_buffer, write_buffer)) {
				tst_brk(TFAIL, "Data read from %s and written "
					 "mismatch", file);
			} else {
				SAFE_CLOSE(otfd);
				tst_res(TPASS, "mount(2) passed with flag MS_SYNCHRONOUS");
				break;
			}
		} else {
			tst_brk(TFAIL | TERRNO, "read() Fails on %s", file);
		}
		break;
	case 4:
		/* Validate MS_REMOUNT flag of mount call */

		TEST(mount(tst_device->dev, MNTPOINT, tst_device->fs_type, MS_REMOUNT, NULL));
		if (TST_RET != 0) {
			tst_brk(TFAIL | TTERRNO, "mount(2) failed to remount");
		} else {
			snprintf(file, PATH_MAX, "%s/tmp2", MNTPOINT);
			TEST(open(file, O_CREAT | O_RDWR, S_IRWXU));
			otfd = TST_RET;
			if (otfd == -1) {
				tst_res(TFAIL, "open(%s) on readonly "
					 "filesystem passed", file);
			} else
				tst_res(TPASS, "mount(2) passed with flag MS_REMOUNT");
		}
		break;
	case 5:
		/* Validate MS_NOSUID flag of mount call */

		snprintf(file, PATH_MAX, "%s/mount03_setuid_test", MNTPOINT);
		TEST(test_ms_nosuid());
		if (TST_RET == 0)
			tst_res(TPASS, "mount(2) passed with flag MS_NOSUID");
		else
			tst_res(TFAIL, "mount(2) failed with flag MS_NOSUID");
		break;
	case 6:
		/* Validate MS_NOATIME flag of mount call */

		snprintf(file, PATH_MAX, "%s/atime", MNTPOINT);
		TST_EXP_FD_SILENT(open(file, O_CREAT | O_RDWR, S_IRWXU));
		otfd = TST_RET;

		SAFE_WRITE(1, otfd, "TEST_MS_NOATIME", 15);

		SAFE_FSTAT(otfd, &file_stat);

		atime = file_stat.st_atime;

		sleep(1);

		SAFE_READ(0, otfd, readbuf, sizeof(readbuf));

		SAFE_FSTAT(otfd, &file_stat);

		if (file_stat.st_atime != atime) {
			tst_res(TFAIL, "access time is updated");
			break;
		}
		tst_res(TPASS, "mount(2) passed with flag MS_NOATIME");
	}
}

static void run(unsigned int n)
{
	struct tcase *tc = &tcases[n];

	TEST(mount(tst_device->dev, MNTPOINT, tst_device->fs_type,
		   tc->flags, NULL));
	test_rwflag(n);

	if (otfd >= 0)
		SAFE_CLOSE(otfd);
	if (tst_is_mounted(MNTPOINT))
		SAFE_UMOUNT(MNTPOINT);
}

static void cleanup(void)
{
	if (otfd > -1)
		SAFE_CLOSE(otfd);
}

static void setup(void)
{
	struct stat file_stat = {0};

	ltpuser = SAFE_GETPWNAM("nobody");
	snprintf(file, PATH_MAX, "%s/mount03_setuid_test", MNTPOINT);
	SAFE_STAT(MNTPOINT, &file_stat);
	if (file_stat.st_mode != SUID_MODE &&
	    chmod(MNTPOINT, SUID_MODE) < 0)
		tst_brk(TBROK, "setuid for setuid_test failed");
}

static struct tst_test test = {
	.tcnt = ARRAY_SIZE(tcases),
	.test = run,
	.setup = setup,
	.cleanup = cleanup,
	.needs_root = 1,
	.format_device = 1,
	.resource_files = (const char *const[]) {
		"mount03_setuid_test",
		NULL,
	},
	.forks_child = 1,
	.mntpoint = MNTPOINT,
	.skip_filesystems = (const char *const []){"vfat", "exfat", "ntfs", NULL},
	.all_filesystems = 1,
};
