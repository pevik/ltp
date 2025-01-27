// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2025 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Test if setxattrat() syscall is correctly raising errors when giving invalid
 * inputs.
 */

#include <sys/xattr.h>

#include "tst_test.h"
#include "lapi/xattr.h"
#include "lapi/syscalls.h"

#define FNAME "ltp_file"
#define XATTR_TEST_KEY "trusted.ltptestkey"
#define XATTR_TEST_VALUE "ltprulez"
#define XATTR_TEST_VALUE_SIZE 8

static struct xattr_args *args;
static struct xattr_args *null_args;
static int invalid_fd = -1;
static int tmpdir_fd = -1;

static struct tcase {
	int *dfd;
	int at_flags;
	struct xattr_args **args;
	size_t args_size;
	int exp_errno;
	char *reason;
} tcases[] = {
	{
		.dfd = &invalid_fd,
		.args = &args,
		.args_size = sizeof(struct xattr_args),
		.exp_errno = EBADF,
		.reason = "Invalid directory file descriptor",
	},
	{
		.dfd = &tmpdir_fd,
		.at_flags = -1,
		.args = &args,
		.args_size = sizeof(struct xattr_args),
		.exp_errno = EINVAL,
		.reason = "Invalid AT flags",
	},
	{
		.dfd = &tmpdir_fd,
		.at_flags = AT_SYMLINK_NOFOLLOW + 1,
		.args = &args,
		.args_size = sizeof(struct xattr_args),
		.exp_errno = EINVAL,
		.reason = "Out of bound AT flags",
	},
	{
		.dfd = &tmpdir_fd,
		.args = &null_args,
		.args_size = sizeof(struct xattr_args),
		.exp_errno = EINVAL,
		.reason = "Invalid arguments",
	},
	{
		.dfd = &tmpdir_fd,
		.args = &args,
		.args_size = SIZE_MAX,
		.exp_errno = E2BIG,
		.reason = "Arguments size is too big",
	},
	{
		.dfd = &tmpdir_fd,
		.args = &args,
		.args_size = sizeof(struct xattr_args) - 1,
		.exp_errno = EINVAL,
		.reason = "Invalid arguments size",
	},
};

static void run(unsigned int i)
{
	struct tcase *tc = &tcases[i];

	args->flags = XATTR_CREATE;
	args->value = XATTR_TEST_VALUE;
	args->size = XATTR_TEST_VALUE_SIZE;

	TST_EXP_FAIL(tst_syscall(__NR_setxattrat,
		tc->dfd, FNAME, tc->at_flags, XATTR_TEST_KEY,
		tc->args, tc->args_size),
		tc->exp_errno, "%s", tc->reason);
}

static void setup(void)
{
	char *tmpdir;

	tmpdir = tst_tmpdir_path();
	tmpdir_fd = SAFE_OPEN(tmpdir, O_DIRECTORY);

	SAFE_TOUCH(FNAME, 0777, NULL);
}

static void cleanup(void)
{
	if (tmpdir_fd != -1)
		SAFE_CLOSE(tmpdir_fd);

	SAFE_UNLINK(FNAME);
}

static struct tst_test test = {
	.test = run,
	.setup = setup,
	.cleanup = cleanup,
	.tcnt = ARRAY_SIZE(tcases),
	.needs_root = 1,
	.needs_tmpdir = 1,
	.bufs = (struct tst_buffers []) {
		{&args, .size = sizeof(struct xattr_args)},
		{},
	}
};
