// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2023 Wei Gao <wegao@suse.com>
 */

/*\
 * [Description]
 *
 * This is a basic test for io_submit RWF_APPEND flag.
 *
 */

#include <linux/aio_abi.h>

#include "config.h"
#include "tst_test.h"
#include "tst_safe_pthread.h"
#include "lapi/syscalls.h"

#define TEST_FILE "test_file"
#define MODE 0777
#define BUF_LEN 10
#define MNTPOINT "mntpoint"

static char *init_buf;
static char *update_buf;
static char *tmp_buf;
static int fd;
static aio_context_t ctx;
static struct iocb iocb;
static struct iocb *iocbs[] = {&iocb};
static volatile int stop;

static inline void io_prep_option(struct iocb *cb, int fd, void *buf,
			size_t count, long long offset, unsigned int opcode)
{
	memset(cb, 0, sizeof(*cb));
	cb->aio_fildes = fd;
	cb->aio_lio_opcode = opcode;
	cb->aio_buf = (uint64_t)buf;
	cb->aio_offset = offset;
	cb->aio_nbytes = count;
	cb->aio_rw_flags = RWF_APPEND;
}

static unsigned int io_submit(void)
{
	struct io_event evbuf;
	struct timespec timeout = { .tv_sec = 1 };

	TST_EXP_VAL_SILENT(tst_syscall(__NR_io_submit, ctx, 1, iocbs), 1);
	TST_EXP_VAL_SILENT(tst_syscall(__NR_io_getevents, ctx, 1, 1, &evbuf,
			&timeout), 1);
}

static void setup(void)
{

	TST_EXP_PASS_SILENT(tst_syscall(__NR_io_setup, 1, &ctx));

	fd = SAFE_OPEN(TEST_FILE, O_RDWR | O_CREAT, MODE);
	SAFE_LSEEK(fd, 0, SEEK_SET);

	memset(init_buf, 0x62, BUF_LEN);
	memset(update_buf, 0x61, BUF_LEN);
	memset(tmp_buf, 0, BUF_LEN);

	io_prep_option(&iocb, fd, update_buf, BUF_LEN, 1, IOCB_CMD_PWRITE);
}

static void cleanup(void)
{
	if (fd > 0)
		SAFE_CLOSE(fd);

	if (tst_syscall(__NR_io_destroy, ctx))
		tst_brk(TBROK | TERRNO, "io_destroy() failed");
}


static void run(void)
{
	int i;

	SAFE_WRITE(0, fd, init_buf, BUF_LEN);
	io_submit();
	SAFE_LSEEK(fd, BUF_LEN, SEEK_SET);
	SAFE_READ(1, fd, tmp_buf, BUF_LEN);
	for (i = 0; i < BUF_LEN; i++) {
		if (tmp_buf[i] != 0x61)
			break;
	}

	if (i != BUF_LEN) {
		tst_res(TFAIL, "buffer wrong at %i have %c expected 'a'",
				i, tmp_buf[i]);
		return;
	}

	tst_res(TPASS, "io_submit wrote %zi bytes successfully "
			"with content 'a' expectedly ", BUF_LEN);
}

static struct tst_test test = {
	.needs_tmpdir = 1,
	.needs_kconfigs = (const char *[]) {
		"CONFIG_AIO=y",
		NULL
	},
	.setup = setup,
	.cleanup = cleanup,
	.test_all = run,
	.max_runtime = 60,
	.mntpoint = MNTPOINT,
	.mount_device = 1,
	.all_filesystems = 1,
	.bufs = (struct tst_buffers []) {
		{&init_buf, .size = BUF_LEN},
		{&update_buf, .size = BUF_LEN},
		{&tmp_buf, .size = BUF_LEN},
		{}
	}
};
