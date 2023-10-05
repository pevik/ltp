// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2011  Red Hat, Inc.
 * Copyright (c) 2023 Marius Kittler <mkittler@suse.de>
 */

/*\
 * [Description]
 *
 * In the user.* namespace, only regular files and directories can
 * have extended attributes. Otherwise getxattr(2) will return -1
 * and set errno to ENODATA.
 *
 * There are 4 test cases:
 * - Get attribute from a FIFO, setxattr(2) should return -1 and
 *    set errno to ENODATA
 * - Get attribute from a char special file, setxattr(2) should
 *    return -1 and set errno to ENODATA
 * - Get attribute from a block special file, setxattr(2) should
 *    return -1 and set errno to ENODATA
 * - Get attribute from a UNIX domain socket, setxattr(2) should
 *    return -1 and set errno to ENODATA
 */

#include "config.h"
#include <sys/types.h>
#include <sys/sysmacros.h>
#include <stdio.h>
#include <stdlib.h>

#include "tst_test.h"

#ifdef HAVE_SYS_XATTR_H
#include <sys/xattr.h>

#include "tst_test_macros.h"

#define MNTPOINT "mntpoint"
#define FNAME MNTPOINT"/getxattr02"
#define XATTR_TEST_KEY "user.testkey"

#define FIFO "getxattr02fifo"
#define CHR  "getxattr02chr"
#define BLK  "getxattr02blk"
#define SOCK "getxattr02sock"

static struct test_case {
	char *fname;
	int mode;
} tcases[] = {
	{ /* case 00, get attr from fifo */
	 .fname = FNAME FIFO,
	 .mode = S_IFIFO,
	},
	{ /* case 01, get attr from char special */
	 .fname = FNAME CHR,
	 .mode = S_IFCHR,
	},
	{ /* case 02, get attr from block special */
	 .fname = FNAME BLK,
	 .mode = S_IFBLK,
	},
	{ /* case 03, get attr from UNIX domain socket */
	 .fname = FNAME SOCK,
	 .mode = S_IFSOCK,
	},
};

static void run(unsigned int i)
{
	char buf[BUFSIZ];
	struct test_case *tc = &tcases[i];
	dev_t dev = tc->mode == S_IFCHR ? makedev(1, 3) : 0u;

	if (mknod(tc->fname, tc->mode | 0777, dev) < 0)
		tst_brk(TBROK | TERRNO, "create %s (mode %i) failed", tc->fname, tc->mode);

	TEST(getxattr(tc->fname, XATTR_TEST_KEY, buf, BUFSIZ));
	if (TST_RET == -1 && TST_ERR == ENODATA)
		tst_res(TPASS | TTERRNO, "expected return value");
	else
		tst_res(TFAIL | TTERRNO, "unexpected return value"
				" - expected errno %d - got", ENODATA);
}

static void setup(void)
{
	/* assert xattr support in the current filesystem */
	SAFE_TOUCH(FNAME, 0644, NULL);
	SAFE_SETXATTR(FNAME, "user.test", "test", 4, XATTR_CREATE);
}

static struct tst_test test = {
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
	.setup = setup,
	.test = run,
	.tcnt = ARRAY_SIZE(tcases)
};

#else
TST_TEST_TCONF("System doesn't have <sys/xattr.h>");
#endif
