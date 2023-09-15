// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2011  Red Hat, Inc.
 * Copyright (c) 2023 Marius Kittler <mkittler@suse.de>
 */

/*\
 * [Description]
 *
 * Basic tests for getxattr(2), there are 3 test cases:
 * 1. Get an non-existing attribute,
 *    getxattr(2) should return -1 and set errno to ENODATA.
 * 2. Buffer size is smaller than attribute value size,
 *    getxattr(2) should return -1 and set errno to ERANGE.
 * 3. getxattr(2) should succeed and return the same value we set
 *    before.
 */

#include "config.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_SYS_XATTR_H
# include <sys/xattr.h>
#endif

#include "tst_test.h"
#include "tst_test_macros.h"

#define MNTPOINT "mntpoint"
#define XATTR_TEST_KEY "user.testkey"
#define XATTR_TEST_VALUE "this is a test value"
#define XATTR_TEST_VALUE_SIZE 20
#define BUFFSIZE 64

static char filename[BUFSIZ];
static char *workdir;

struct test_case {
	char *fname;
	char *key;
	char value[BUFFSIZE];
	size_t size;
	int exp_err;
} tcases[] = {
	{ /* case 00, get non-existing attribute */
	 .fname = filename,
	 .key = "user.nosuchkey",
	 .value = {0},
	 .size = BUFFSIZE - 1,
	 .exp_err = ENODATA,
	},
	{ /* case 01, small value buffer */
	 .fname = filename,
	 .key = XATTR_TEST_KEY,
	 .value = {0},
	 .size = 1,
	 .exp_err = ERANGE,
	},
	{ /* case 02, get existing attribute */
	 .fname = filename,
	 .key = XATTR_TEST_KEY,
	 .value = {0},
	 .size = BUFFSIZE - 1,
	 .exp_err = 0,
	},
};

static void run(unsigned int i)
{
#ifdef HAVE_SYS_XATTR_H
	SAFE_CHDIR(workdir);

	/* create test file and set xattr */
	struct test_case *tc = &tcases[i];
	snprintf(tc->fname, BUFSIZ, "getxattr01testfile-%u", i);
	int fd = SAFE_CREAT(tc->fname, 0644);
	SAFE_CLOSE(fd);
	TEST(setxattr(tc->fname, XATTR_TEST_KEY, XATTR_TEST_VALUE,
				  strlen(XATTR_TEST_VALUE), XATTR_CREATE));
	if (TST_RET < 0) {
		if (TST_ERR == ENOTSUP) {
			tst_res(TCONF, "no xattr support in file system");
			return;
		}
		tst_res(TFAIL | TTERRNO, "unexpected setxattr() return code");
		return;
	}
	tst_res(TPASS | TTERRNO, "expected setxattr() return code");

	/* read xattr back */
	TEST(getxattr(tc->fname, tc->key, tc->value, tc->size));
	if (TST_ERR == tc->exp_err) {
		tst_res(TPASS | TTERRNO, "expected getxattr() return code");
	} else {
		tst_res(TFAIL | TTERRNO, "unexpected getxattr() return code"
				" - expected errno %d", tc->exp_err);
	}

	/* verify the value for non-error test cases */
	if (tc->exp_err) {
		return;
	}
	if (TST_RET != XATTR_TEST_VALUE_SIZE) {
		tst_res(TFAIL,
				"getxattr() returned wrong size %ld expected %d",
				TST_RET, XATTR_TEST_VALUE_SIZE);
		return;
	}
	if (memcmp(tc->value, XATTR_TEST_VALUE, XATTR_TEST_VALUE_SIZE))
		tst_res(TFAIL, "wrong value, expected \"%s\" got \"%s\"",
				XATTR_TEST_VALUE, tc->value);
	else
		tst_res(TPASS, "right value");
#endif
}

static void setup(void)
{
#ifdef HAVE_SYS_XATTR_H
	char *cwd = SAFE_GETCWD(NULL, 0);
	workdir = SAFE_MALLOC(strlen(cwd) + strlen(MNTPOINT) + 2);
	sprintf(workdir, "%s/%s", cwd, MNTPOINT);
	free(cwd);
#else
	tst_brk(TCONF, "<sys/xattr.h> does not exist.");
#endif
}

static struct tst_test test = {
#ifdef HAVE_SYS_XATTR_H
	.all_filesystems = 1,
	.needs_root = 1,
	.mntpoint = MNTPOINT,
	.mount_device = 1,
	.skip_filesystems = (const char *const []) {
			"exfat",
			"tmpfs",
			"ramfs",
			"nfs",
			"vfat",
			NULL
	},
#endif
	.setup = setup,
	.test = run,
	.tcnt = ARRAY_SIZE(tcases)
};
