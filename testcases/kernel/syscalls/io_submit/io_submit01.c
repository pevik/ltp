// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) Crackerjack Project., 2007
 * Ported from Crackerjack to LTP by Masatake YAMATO <yamato@redhat.com>
 * Copyright (c) 2011-2017 Cyril Hrubis <chrubis@suse.cz>
 */

/*\
 * [Description]
 *
 * - io_submit(2) fails and returns EINVAL if ctx is invalid;
 * - io_submit(2) fails and returns EINVAL if nr is invalid;
 * - io_submit(2) fails and returns EFAULT if iocbpp pointer is invalid;
 * - io_submit(2) fails and returns EBADF if fd is invalid;
 * - io_submit(2) should work fine if no-op;
 */

#include <linux/aio_abi.h>

#include "config.h"
#include "tst_test.h"
#include "lapi/syscalls.h"

static aio_context_t ctx;
static aio_context_t invalid_ctx;
static char buf[100];

static struct iocb iocb;
static struct iocb *iocbs[] = {&iocb};

static struct iocb inv_fd_iocb;
static struct iocb *inv_fd_iocbs[] = {&inv_fd_iocb};

static int rdonly_fd;
static struct iocb rdonly_fd_iocb;
static struct iocb *rdonly_fd_iocbs[] = {&rdonly_fd_iocb};

static int wronly_fd;
static struct iocb wronly_fd_iocb;
static struct iocb *wronly_fd_iocbs[] = {&wronly_fd_iocb};

static struct iocb zero_buf_iocb;
static struct iocb *zero_buf_iocbs[] = {&zero_buf_iocb};

static struct iocb *zero_iocbs[1];

static struct tcase {
	aio_context_t *ctx;
	long nr;
	struct iocb **iocbs;
	int exp_errno;
	const char *desc;
} tc[] = {
	/* Invalid ctx */
	{&invalid_ctx, 1, iocbs, EINVAL, "invalid ctx"},
	/* Invalid nr */
	{&ctx, -1, iocbs, EINVAL, "invalid nr"},
	/* Invalid pointer */
	{&ctx, 1, (void*)-1, EFAULT, "invalid iocbpp pointer"},
	{&ctx, 1, zero_iocbs, EFAULT, "NULL iocb pointers"},
	/* Invalid fd */
	{&ctx, 1, inv_fd_iocbs, EBADF, "invalid fd"},
	{&ctx, 1, rdonly_fd_iocbs, EBADF, "readonly fd for write"},
	{&ctx, 1, wronly_fd_iocbs, EBADF, "writeonly fd for read"},
	/* No-op but should work fine */
	{&ctx, 1, zero_buf_iocbs, 0, "zero buf size"},
	{&ctx, 0, NULL, 0, "zero nr"},
};

static inline void io_prep_option(struct iocb *cb, int fd, void *buf,
			size_t count, long long offset, unsigned opcode)
{
	memset(cb, 0, sizeof(*cb));
	cb->aio_fildes = fd;
	cb->aio_lio_opcode = opcode;
	cb->aio_buf = (uint64_t)buf;
	cb->aio_offset = offset;
	cb->aio_nbytes = count;
}

static void setup(void)
{
	TST_EXP_PASS(tst_syscall(__NR_io_setup, 1, &ctx));
	io_prep_option(&inv_fd_iocb, -1, buf, sizeof(buf), 0, IOCB_CMD_PREAD);

	rdonly_fd = SAFE_OPEN("rdonly_file", O_RDONLY | O_CREAT, 0777);
	io_prep_option(&rdonly_fd_iocb, rdonly_fd, buf, sizeof(buf), 0, IOCB_CMD_PWRITE);

	io_prep_option(&zero_buf_iocb, rdonly_fd, buf, 0, 0, IOCB_CMD_PREAD);

	wronly_fd = SAFE_OPEN("wronly_file", O_WRONLY | O_CREAT, 0777);
	io_prep_option(&wronly_fd_iocb, wronly_fd, buf, sizeof(buf), 0, IOCB_CMD_PREAD);
}

static void cleanup(void)
{
	if (rdonly_fd > 0)
		SAFE_CLOSE(rdonly_fd);
	if (wronly_fd > 0)
		SAFE_CLOSE(wronly_fd);
}

static void run(unsigned int i)
{
	TEST(tst_syscall(__NR_io_submit, *tc[i].ctx, tc[i].nr, tc[i].iocbs));
	tst_res(TST_ERR == tc[i].exp_errno ? TPASS : TFAIL,
		"io_submit(2) with %s returns %s, expected %s",
		tc[i].desc, tst_strerrno(TST_ERR), tst_strerrno(tc[i].exp_errno));
}

static struct tst_test test = {
	.setup = setup,
	.cleanup = cleanup,
	.test = run,
	.tcnt = ARRAY_SIZE(tc),
	.needs_tmpdir = 1,
};
