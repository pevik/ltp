// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2008 Michael Kerrisk <mtk.manpages@gmail.com>
 * Copyright (c) 2008 Subrata Modak <subrata@linux.vnet.ibm.com>
 * Copyright (c) 2020 Viresh Kumar <viresh.kumar@linaro.org>
 *
 * Basic utimnsat() test.
 */

#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include "tst_test.h"
#include "lapi/syscalls.h"

#define UTIME_NOW	((1l << 30) - 1l)
#define UTIME_OMIT	((1l << 30) - 2l)

#define TEST_FILE	"test_file"
#define TEST_DIR	"test_dir"

static void *bad_addr;

struct mytime {
	long access_tv_sec;
	long access_tv_nsec;
	long mod_tv_sec;
	long mod_tv_nsec;
};

static struct mytime tnn = {0, UTIME_NOW, 0, UTIME_NOW}, *time_nn = &tnn;
static struct mytime too = {0, UTIME_OMIT, 0, UTIME_OMIT}, *time_oo = &too;
static struct mytime tno = {0, UTIME_NOW, 0, UTIME_OMIT}, *time_no = &tno;
static struct mytime ton = {0, UTIME_OMIT, 0, UTIME_NOW}, *time_on = &ton;
static struct mytime t11 = {1, 1, 1, 1}, *time_11 = &t11;

struct test_case {
	int dirfd;
	char *pathname;
	struct mytime **mytime;
	int flags;
	int oflags;
	char *attr;
	char *rattr;
	int mode;
	int exp_err;
} tcase[] = {
	/* Testing read-only file */
	{AT_FDCWD, TEST_FILE, NULL, 0, O_RDONLY, NULL, NULL, 0400, 0},
	{AT_FDCWD, TEST_FILE, &time_nn, 0, O_RDONLY, NULL, NULL, 0400, 0},
	{AT_FDCWD, TEST_FILE, &time_oo, 0, O_RDONLY, NULL, NULL, 0400, 0},
	{AT_FDCWD, TEST_FILE, &time_no, 0, O_RDONLY, NULL, NULL, 0400, 0},
	{AT_FDCWD, TEST_FILE, &time_on, 0, O_RDONLY, NULL, NULL, 0400, 0},
	{AT_FDCWD, TEST_FILE, &time_11, 0, O_RDONLY, NULL, NULL, 0400, 0},

	/* Testing writable file */
	{AT_FDCWD, TEST_FILE, NULL, 0, O_RDONLY, NULL, NULL, 0666, 0},
	{AT_FDCWD, TEST_FILE, &time_nn, 0, O_RDONLY, NULL, NULL, 0666, 0},
	{AT_FDCWD, TEST_FILE, &time_oo, 0, O_RDONLY, NULL, NULL, 0666, 0},
	{AT_FDCWD, TEST_FILE, &time_no, 0, O_RDONLY, NULL, NULL, 0666, 0},
	{AT_FDCWD, TEST_FILE, &time_on, 0, O_RDONLY, NULL, NULL, 0666, 0},
	{AT_FDCWD, TEST_FILE, &time_11, 0, O_RDONLY, NULL, NULL, 0666, 0},

	/* Testing append-only file */
	{AT_FDCWD, TEST_FILE, NULL, 0, O_RDONLY, "+a", "-a", 0600, 0},
	{AT_FDCWD, TEST_FILE, &time_nn, 0, O_RDONLY, "+a", "-a", 0600, 0},
	{AT_FDCWD, TEST_FILE, &time_oo, 0, O_RDONLY, "+a", "-a", 0600, 0},
	{AT_FDCWD, TEST_FILE, &time_no, 0, O_RDONLY, "+a", "-a", 0600, EPERM},
	{AT_FDCWD, TEST_FILE, &time_on, 0, O_RDONLY, "+a", "-a", 0600, EPERM},
	{AT_FDCWD, TEST_FILE, &time_11, 0, O_RDONLY, "+a", "-a", 0600, EPERM},

	/* Testing immutable file */
	{AT_FDCWD, TEST_FILE, NULL, 0, O_RDONLY, "+i", "-i", 0600, -1},
	{AT_FDCWD, TEST_FILE, &time_nn, 0, O_RDONLY, "+i", "-i", 0600, -1},
	{AT_FDCWD, TEST_FILE, &time_oo, 0, O_RDONLY, "+i", "-i", 0600, 0},
	{AT_FDCWD, TEST_FILE, &time_no, 0, O_RDONLY, "+i", "-i", 0600, EPERM},
	{AT_FDCWD, TEST_FILE, &time_on, 0, O_RDONLY, "+i", "-i", 0600, EPERM},
	{AT_FDCWD, TEST_FILE, &time_11, 0, O_RDONLY, "+i", "-i", 0600, EPERM},

	/* Testing immutable-append-only file */
	{AT_FDCWD, TEST_FILE, NULL, 0, O_RDONLY, "+ai", "-ai", 0600, -1},
	{AT_FDCWD, TEST_FILE, &time_nn, 0, O_RDONLY, "+ai", "-ai", 0600, -1},
	{AT_FDCWD, TEST_FILE, &time_oo, 0, O_RDONLY, "+ai", "-ai", 0600, 0},
	{AT_FDCWD, TEST_FILE, &time_no, 0, O_RDONLY, "+ai", "-ai", 0600, EPERM},
	{AT_FDCWD, TEST_FILE, &time_on, 0, O_RDONLY, "+ai", "-ai", 0600, EPERM},
	{AT_FDCWD, TEST_FILE, &time_11, 0, O_RDONLY, "+ai", "-ai", 0600, EPERM},

	/* Other failure tests */
	{AT_FDCWD, TEST_FILE, NULL, 0, O_RDONLY, NULL, NULL, 0400, EFAULT},
	{AT_FDCWD, NULL, &time_nn, 0, O_RDONLY, NULL, NULL, 0400, EFAULT},
	{-1, NULL, &time_nn, AT_SYMLINK_NOFOLLOW, O_RDONLY, NULL, NULL, 0400, EINVAL},
	{-1, TEST_FILE, &time_nn, 0, O_RDONLY, NULL, NULL, 0400, ENOENT},
};

static inline int sys_utimensat(int dirfd, const char *pathname,
				const struct timespec times[2], int flags)
{
	return tst_syscall(__NR_utimensat, dirfd, pathname, times, flags);
}

static void setup(void)
{
	bad_addr = tst_get_bad_addr(NULL);
}

static void update_error(struct test_case *tc)
{
	if (tc->exp_err != -1)
		return;

	/*
	 * Starting with 4.8.0 operations on immutable files return EPERM
	 * instead of EACCES.
	 * This patch has also been merged to stable 4.4 with
	 * b3b4283 ("vfs: move permission checking into notify_change() for utimes(NULL)")
	 */
	if (tst_kvercmp(4, 4, 27) < 0)
		tc->exp_err = EACCES;
	else
		tc->exp_err = EPERM;
}

static int run_command(char *command, char *option, char *file)
{
	const char *const cmd[] = {command, option, file, NULL};

	return tst_cmd(cmd, NULL, NULL, TST_CMD_PASS_RETVAL);
}

static void run(unsigned int i)
{
	struct test_case *tc = &tcase[i];
	struct timespec ts[2];
	void *tsp = NULL;
	char *pathname = NULL;
	int fd = AT_FDCWD;
	struct stat sb;

	SAFE_MKDIR(TEST_DIR, 0700);
	SAFE_TOUCH(TEST_FILE, 0666, NULL);
	update_error(tc);

	if (tc->dirfd != AT_FDCWD)
		fd = SAFE_OPEN(TEST_DIR, tc->oflags);

	if (tc->mytime) {
		struct mytime *mytime = *tc->mytime;

		ts[0].tv_sec = mytime->access_tv_sec;
		ts[0].tv_nsec = mytime->access_tv_nsec;
		ts[1].tv_sec = mytime->mod_tv_sec;
		ts[1].tv_nsec = mytime->mod_tv_nsec;
		tsp = ts;
	} else if (tc->exp_err == EFAULT) {
		tsp = bad_addr;
	}

	if (tc->pathname) {
		pathname = tc->pathname;
		SAFE_CHMOD(tc->pathname, tc->mode);
		if (tc->attr)
			run_command("chattr", tc->attr, tc->pathname);
	} else if (tc->exp_err == EFAULT) {
		pathname = bad_addr;
	}

	TEST(sys_utimensat(fd, pathname, tsp, tc->flags));
	if (TST_RET) {
		if (!tc->exp_err) {
			tst_res(TFAIL | TTERRNO, "%2d: utimensat() failed", i);
		} else if (tc->exp_err == TST_ERR) {
			tst_res(TPASS | TTERRNO, "%2d: utimensat() failed expectedly", i);
		} else {
			tst_res(TFAIL | TTERRNO, "%2d: utimensat() failed with incorrect error, expected %s",
				i, tst_strerrno(tc->exp_err));
		}
	} else if (tc->exp_err) {
		tst_res(TFAIL, "%2d: utimensat() passed unexpectedly", i);
	} else {
		TEST(stat(tc->pathname ? tc->pathname : TEST_DIR, &sb));
		tst_res(TPASS, "%2d: utimensat() passed: access time: %lu %lu", i,
			sb.st_atime, sb.st_mtime);
	}

	if (tc->rattr != NULL)
		run_command("chattr", tc->rattr, tc->pathname);

	if (fd != AT_FDCWD)
		SAFE_CLOSE(fd);

	SAFE_RMDIR(TEST_DIR);
}

static struct tst_test test = {
	.test = run,
	.tcnt = ARRAY_SIZE(tcase),
	.setup = setup,
	.needs_root = 1,
	.needs_tmpdir = 1,
};
