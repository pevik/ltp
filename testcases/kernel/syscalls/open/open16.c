// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2026 Wei Gao <wegao@suse.com>
 */

/*\
 * Verify restricted opening (:manpage:`open(2)` and :manpage:`openat(2)`) of
 * FIFOs and regular files in sticky directories. This test covers the positive
 * case where access is allowed when protection is disabled (level 0), and the
 * negative cases where access is disallowed (EACCES) in world-writable (level
 * 1) or group-writable (level 2) sticky directories when the file is not owned
 * by the opener.
 *
 * This test requires root to modify /proc/sys/fs/protected_* sysctls and
 * to manage file ownership and permissions in sticky directories.
 */

#include <pwd.h>
#include <stdlib.h>
#include "tst_test.h"
#include "tst_safe_file_at.h"
#include "tst_uid.h"

#define DIR "ltp_tmp_check1"
#define TEST_FILE "test_file_1"
#define TEST_FIFO "test_fifo_1"
#define PROTECTED_REGULAR "/proc/sys/fs/protected_regular"
#define PROTECTED_FIFOS "/proc/sys/fs/protected_fifos"
#define TEST_FIFO_PATH DIR "/" TEST_FIFO

static int dir_fd = -1;
static uid_t uid1, uid2;
static gid_t gid1;

static struct tcase {
	char *level;
	int exp_errno;
	uid_t owner_uid;
	int use_nobody_gid;
	mode_t dir_mode;
} tcases[] = {
	{"0", 0,      0,  0, 0777 | S_ISVTX},
	{"1", EACCES, 0,  0, 0777 | S_ISVTX},
	{"2", EACCES, -1, 1, 0030 | S_ISVTX},
};

static void verify_open(unsigned int n)
{
	struct tcase *tc = &tcases[n];
	pid_t pid;

	SAFE_FILE_PRINTF(PROTECTED_REGULAR, "%s", tc->level);
	SAFE_FILE_PRINTF(PROTECTED_FIFOS, "%s", tc->level);

	if (tc->owner_uid != (uid_t)-1 || tc->use_nobody_gid) {
		gid_t gid = tc->use_nobody_gid ? gid1 : 0;

		SAFE_CHOWN(DIR, tc->owner_uid, gid);
	}

	if (tc->dir_mode)
		SAFE_CHMOD(DIR, tc->dir_mode);

	pid = SAFE_FORK();
	if (!pid) {
		SAFE_SETGID(gid1);
		SAFE_SETUID(uid2);

		if (tc->exp_errno) {
			TST_EXP_FAIL2(openat(dir_fd, TEST_FILE, O_RDWR | O_CREAT, 0777),
				      tc->exp_errno, "openat %s (Level %s)", TEST_FILE, tc->level);
			TST_EXP_FAIL2(open(TEST_FIFO_PATH, O_RDWR | O_CREAT, 0777),
				      tc->exp_errno, "open %s (Level %s)", TEST_FIFO, tc->level);
		} else {
			int fd = TST_EXP_FD(openat(dir_fd, TEST_FILE, O_CREAT | O_RDWR, 0777));

			if (TST_PASS)
				SAFE_CLOSE(fd);

			fd = TST_EXP_FD(open(TEST_FIFO_PATH, O_RDWR | O_CREAT, 0777));
			if (TST_PASS)
				SAFE_CLOSE(fd);
		}

		exit(0);
	}
}

static void setup(void)
{
	struct passwd *pw;

	pw = SAFE_GETPWNAM("nobody");
	uid1 = pw->pw_uid;
	gid1 = pw->pw_gid;
	uid2 = tst_get_free_uid(uid1);

	umask(0);
	SAFE_MKDIR(DIR, 0777 | S_ISVTX);
	dir_fd = SAFE_OPEN(DIR, O_DIRECTORY);

	int fd = SAFE_OPENAT(dir_fd, TEST_FILE, O_CREAT | O_RDWR, 0777);

	SAFE_CLOSE(fd);
	SAFE_MKFIFO(TEST_FIFO_PATH, 0777);
	SAFE_CHOWN(TEST_FIFO_PATH, uid1, gid1);
	SAFE_CHOWN(DIR "/" TEST_FILE, uid1, gid1);
}

static void cleanup(void)
{
	if (dir_fd != -1)
		SAFE_CLOSE(dir_fd);
}

static struct tst_test test = {
	.setup = setup,
	.cleanup = cleanup,
	.needs_root = 1,
	.tcnt = ARRAY_SIZE(tcases),
	.test = verify_open,
	.needs_tmpdir = 1,
	.forks_child = 1,
	.save_restore = (const struct tst_path_val[]) {
		{PROTECTED_REGULAR, NULL, TST_SR_TCONF},
		{PROTECTED_FIFOS, NULL, TST_SR_TCONF},
		{}
	},
	.tags = (const struct tst_tag[]) {
		{"linux-git", "30aba6656f61"},
		{}
	}
};
