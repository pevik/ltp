// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2011 Red Hat, Inc.
 * Copyright (c) Linux Test Project, 2011-2024
 */

/*\
 * [Description]
 *
 * In the user.* namespace, only regular files and directories can
 * have extended attributes. Otherwise setxattr(2) will return -1
 * and set errno to EPERM.
 *
 * - SUCCEED - set attribute to a regular file
 * - SUCCEED - set attribute to a directory
 * - EEXIST - set attribute to a symlink which points to the regular file
 * - EPERM - set attribute to a FIFO
 * - EPERM - set attribute to a char special file
 * - EPERM - set attribute to a block special file
 * - EPERM - set attribute to a UNIX domain socket
 */

#include "config.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_SYS_XATTR_H
# include <sys/xattr.h>
#endif
#include "tst_test.h"
#include "lapi/syscalls.h"
#include "lapi/xattr.h"
#include "lapi/fcntl.h"

#ifdef HAVE_SYS_XATTR_H
#define XATTR_TEST_KEY "user.testkey"
#define XATTR_TEST_VALUE "this is a test value"
#define XATTR_TEST_VALUE_SIZE 20

#define OFFSET    10
#define FILENAME "setxattr02testfile"
#define DIRNAME  "setxattr02testdir"
#define SYMLINK  "setxattr02symlink"
#define FIFO     "setxattr02fifo"
#define CHR      "setxattr02chr"
#define BLK      "setxattr02blk"
#define SOCK     "setxattr02sock"

static int tmpdir_fd = -1;

struct test_case {
	char *fname;
	char *key;
	char *value;
	size_t size;
	int flags;
	int exp_err;
	int needskeyset;
};
static struct test_case tc[] = {
	{			/* case 00, set attr to reg */
	 .fname = FILENAME,
	 .key = XATTR_TEST_KEY,
	 .value = XATTR_TEST_VALUE,
	 .size = XATTR_TEST_VALUE_SIZE,
	 .flags = XATTR_CREATE,
	 .exp_err = 0,
	 },
	{			/* case 01, set attr to dir */
	 .fname = DIRNAME,
	 .key = XATTR_TEST_KEY,
	 .value = XATTR_TEST_VALUE,
	 .size = XATTR_TEST_VALUE_SIZE,
	 .flags = XATTR_CREATE,
	 .exp_err = 0,
	 },
	{			/* case 02, set attr to symlink */
	 .fname = SYMLINK,
	 .key = XATTR_TEST_KEY,
	 .value = XATTR_TEST_VALUE,
	 .size = XATTR_TEST_VALUE_SIZE,
	 .flags = XATTR_CREATE,
	 .exp_err = EEXIST,
	 .needskeyset = 1,
	 },
	{			/* case 03, set attr to fifo */
	 .fname = FIFO,
	 .key = XATTR_TEST_KEY,
	 .value = XATTR_TEST_VALUE,
	 .size = XATTR_TEST_VALUE_SIZE,
	 .flags = XATTR_CREATE,
	 .exp_err = EPERM,
	 },
	{			/* case 04, set attr to character special */
	 .fname = CHR,
	 .key = XATTR_TEST_KEY,
	 .value = XATTR_TEST_VALUE,
	 .size = XATTR_TEST_VALUE_SIZE,
	 .flags = XATTR_CREATE,
	 .exp_err = EPERM,
	 },
	{			/* case 05, set attr to block special */
	 .fname = BLK,
	 .key = XATTR_TEST_KEY,
	 .value = XATTR_TEST_VALUE,
	 .size = XATTR_TEST_VALUE_SIZE,
	 .flags = XATTR_CREATE,
	 .exp_err = EPERM,
	 },
	{			/* case 06, set attr to socket */
	 .fname = SOCK,
	 .key = XATTR_TEST_KEY,
	 .value = XATTR_TEST_VALUE,
	 .size = XATTR_TEST_VALUE_SIZE,
	 .flags = XATTR_CREATE,
	 .exp_err = EPERM,
	 },
};

static void verify_setxattr(unsigned int i)
{
	char *sysname;

	/* some tests might require existing keys for each iteration */
	if (tc[i].needskeyset) {
		SAFE_SETXATTR(tc[i].fname, tc[i].key, tc[i].value, tc[i].size,
			XATTR_CREATE);
	}

	if (tst_variant) {
		sysname = "setxattrat";

		struct xattr_args args = {
			.value = tc[i].value,
			.size = tc[i].size,
			.flags = tc[i].flags,
		};

		int at_flags = tc[i].needskeyset ? 0 : AT_SYMLINK_NOFOLLOW;

		TEST(tst_syscall(__NR_setxattrat,
			tmpdir_fd, tc[i].fname, at_flags,
			tc[i].key, &args, sizeof(args)));
	} else {
		sysname = "setxattr";

		TEST(setxattr(tc[i].fname, tc[i].key, tc[i].value, tc[i].size,
				tc[i].flags));
	}

	if (TST_RET == -1 && TST_ERR == EOPNOTSUPP)
		tst_brk(TCONF, "%s(2) not supported", sysname);

	/* success */

	if (!tc[i].exp_err) {
		if (TST_RET) {
			tst_res(TFAIL | TTERRNO,
				"%s(2) on %s failed with %li",
				sysname, tc[i].fname + OFFSET, TST_RET);
			return;
		}

		/* this is needed for subsequent iterations */
		SAFE_REMOVEXATTR(tc[i].fname, tc[i].key);

		tst_res(TPASS, "%s(2) on %s passed",
				sysname, tc[i].fname + OFFSET);
		return;
	}

	if (TST_RET == 0) {
		tst_res(TFAIL, "%s(2) on %s passed unexpectedly",
				sysname, tc[i].fname + OFFSET);
		return;
	}

	/* fail */

	if (tc[i].exp_err != TST_ERR) {
		tst_res(TFAIL | TTERRNO,
				"%s(2) on %s should have failed with %s",
				sysname, tc[i].fname + OFFSET,
				tst_strerrno(tc[i].exp_err));
		return;
	}

	/* key might have been added AND test might have failed, remove it */
	if (tc[i].needskeyset)
		SAFE_REMOVEXATTR(tc[i].fname, tc[i].key);

	tst_res(TPASS | TTERRNO, "%s(2) on %s failed",
			sysname, tc[i].fname + OFFSET);
}

static void setup(void)
{
	dev_t dev = makedev(1, 3);

	SAFE_TOUCH(FILENAME, 0644, NULL);
	SAFE_MKDIR(DIRNAME, 0644);
	SAFE_SYMLINK(FILENAME, SYMLINK);
	SAFE_MKNOD(FIFO, S_IFIFO | 0777, 0);
	SAFE_MKNOD(CHR, S_IFCHR | 0777, dev);
	SAFE_MKNOD(BLK, S_IFBLK | 0777, 0);
	SAFE_MKNOD(SOCK, S_IFSOCK | 0777, 0);

	tmpdir_fd = SAFE_OPEN(tst_tmpdir_path(), O_DIRECTORY);
}

static void cleanup(void)
{
	if (tmpdir_fd != -1)
		SAFE_CLOSE(tmpdir_fd);

	SAFE_UNLINK(FILENAME);
	SAFE_RMDIR(DIRNAME);
	SAFE_UNLINK(SYMLINK);
	SAFE_UNLINK(FIFO);
	SAFE_UNLINK(CHR);
	SAFE_UNLINK(BLK);
	SAFE_UNLINK(SOCK);
}

static struct tst_test test = {
	.setup = setup,
	.cleanup = cleanup,
	.test = verify_setxattr,
	.tcnt = ARRAY_SIZE(tc),
	.test_variants = 2,
	.needs_tmpdir = 1,
	.needs_root = 1,
};

#else /* HAVE_SYS_XATTR_H */
TST_TEST_TCONF("<sys/xattr.h> does not exist");
#endif
