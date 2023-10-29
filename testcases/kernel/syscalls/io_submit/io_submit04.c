// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2023 Wei Gao <wegao@suse.com>
 */

/*\
 * [Description]
 *
 * Similarily to the preadv2, this is a basic test for io_submit
 * RWF_NOWAIT flag, we are attempting to force io_submit return
 * EAGAIN with thread concurently running threads.
 *
 */

#include <linux/aio_abi.h>

#include "config.h"
#include "tst_test.h"
#include "tst_safe_pthread.h"
#include "lapi/syscalls.h"

#define TEST_FILE "test_file"
#define MODE 0777
#define BUF_LEN 1000000
#define MNTPOINT "mntpoint"

static char *w_buf;
static char *r_buf;
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
	cb->aio_buf = (uint64_t)r_buf;
	cb->aio_offset = offset;
	cb->aio_nbytes = count;
	cb->aio_rw_flags = RWF_NOWAIT;
}

static void write_test(void)
{
	SAFE_LSEEK(fd, 0, SEEK_SET);
	SAFE_WRITE(SAFE_WRITE_ALL, fd, r_buf, BUF_LEN);
}

static void *writer_thread(void *unused)
{
	while (!stop)
		write_test();

	return unused;
}

static void drop_caches(void)
{
	SAFE_FILE_PRINTF("/proc/sys/vm/drop_caches", "3");
}

static void *cache_dropper(void *unused)
{
	unsigned int drop_cnt = 0;

	while (!stop) {
		drop_caches();
		drop_cnt++;
	}

	tst_res(TINFO, "Cache dropped %u times", drop_cnt);

	return unused;
}

static unsigned int io_submit(void)
{
	struct io_event evbuf;
	struct timespec timeout = { .tv_sec = 1 };

	TST_EXP_VAL_SILENT(tst_syscall(__NR_io_submit, ctx, 1, iocbs), 1);

	TST_EXP_VAL_SILENT(tst_syscall(__NR_io_getevents, ctx, 1, 1, &evbuf,
			&timeout), 1);

	if (evbuf.res == -EAGAIN)
		return 1;
	else
		return 0;
}

static void *nowait_reader(void *unused LTP_ATTRIBUTE_UNUSED)
{
	unsigned int eagains_cnt = 0;

	while (!stop) {
		if (eagains_cnt >= 100)
			stop = 1;
		eagains_cnt = eagains_cnt + io_submit();
	}

	return (void *)(long)eagains_cnt;
}

static void setup(void)
{

	TST_EXP_PASS_SILENT(tst_syscall(__NR_io_setup, 1, &ctx));

	fd = SAFE_OPEN(TEST_FILE, O_RDWR | O_CREAT, MODE);

	memset(w_buf, 'a', BUF_LEN);
	memset(r_buf, 'b', BUF_LEN);

	io_prep_option(&iocb, fd, r_buf, BUF_LEN, 0, IOCB_CMD_PREAD);
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

	pthread_t reader, dropper, writer;
	void *eagains;

	stop = 0;

	SAFE_PTHREAD_CREATE(&dropper, NULL, cache_dropper, NULL);
	SAFE_PTHREAD_CREATE(&reader, NULL, nowait_reader, NULL);
	SAFE_PTHREAD_CREATE(&writer, NULL, writer_thread, NULL);

	while (!stop && tst_remaining_runtime())
		usleep(100000);

	stop = 1;

	SAFE_PTHREAD_JOIN(reader, &eagains);
	SAFE_PTHREAD_JOIN(dropper, NULL);
	SAFE_PTHREAD_JOIN(writer, NULL);

	if (eagains)
		tst_res(TPASS, "Got some EAGAIN");
	else
		tst_res(TFAIL, "Haven't got EAGAIN");

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
		{&w_buf, .size = BUF_LEN},
		{&r_buf, .size = BUF_LEN},
		{}
	}
};
