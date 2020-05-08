// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) Crackerjack Project., 2007-2008, Hitachi, Ltd
 * Copyright (c) 2017 Petr Vorel <pvorel@suse.cz>
 *
 * Authors:
 * Takahiro Yasui <takahiro.yasui.mp@hitachi.com>,
 * Yumiko Sugita <yumiko.sugita.yf@hitachi.com>,
 * Satoshi Fujiwara <sa-fuji@sdl.hitachi.co.jp>
 */

#include <errno.h>
#include <limits.h>
#include "lapi/abisize.h"

static int fd, fd_root, fd_nonblock, fd_maxint = INT_MAX - 1, fd_invalid = -1;

#include "mq_timed.h"

static struct tst_ts ts;

static struct test_case tcase[] = {
	{
		.fd = &fd,
		.len = 0,
		.send = 1,
		.ret = 0,
		.err = 0,
	},
	{
		.fd = &fd,
		.len = 1,
		.send = 1,
		.ret = 0,
		.err = 0,
	},
	{
		.fd = &fd,
		.len = MAX_MSGSIZE,
		.send = 1,
		.ret = 0,
		.err = 0,
	},
	{
		.fd = &fd,
		.len = 1,
		.send = 1,
		.prio = MQ_PRIO_MAX - 1,
		.ret = 0,
		.err = 0,
	},
	{
		.fd = &fd,
		.invalid_msg = 1,
		.len = 0,
		.send = 1,
		.ret = -1,
		.err = EMSGSIZE,
	},
	{
		.fd = &fd_invalid,
		.len = 0,
		.ret = -1,
		.err = EBADF,
	},
	{
		.fd = &fd_maxint,
		.len = 0,
		.ret = -1,
		.err = EBADF,
	},
	{
		.fd = &fd_root,
		.len = 0,
		.ret = -1,
		.err = EBADF,
	},
	{
		.fd = &fd_nonblock,
		.len = 16,
		.ret = -1,
		.err = EAGAIN,
	},
	{
		.fd = &fd,
		.len = 16,
		.tv_sec = -1,
		.tv_nsec = 0,
		.rq = &ts,
		.ret = -1,
		.err = EINVAL,
	},
	{
		.fd = &fd,
		.len = 16,
		.tv_sec = 0,
		.tv_nsec = -1,
		.rq = &ts,
		.ret = -1,
		.err = EINVAL,
	},
	{
		.fd = &fd,
		.len = 16,
		.tv_sec = 0,
		.tv_nsec = 1000000000,
		.rq = &ts,
		.ret = -1,
		.err = EINVAL,
	},
	{
		.fd = &fd,
		.len = 16,
		.timeout = 1,
		.ret = -1,
		.rq = &ts,
		.err = ETIMEDOUT,
	},
	{
		.fd = &fd,
		.len = 16,
		.signal = 1,
		.rq = &ts,
		.ret = -1,
		.err = EINTR,
	},
};

static struct test_variants {
	int (*send)(mqd_t mqdes, const char *msg_ptr, size_t msg_len,
		    unsigned int msg_prio, void *abs_timeout);
	ssize_t (*receive)(mqd_t mqdes, char *msg_ptr, size_t msg_len,
			   unsigned int *msg_prio, void *abs_timeout);

	int (*gettime)(clockid_t clk_id, void *ts);
	enum tst_ts_type type;
	char *desc;
} variants[] = {
#if defined(TST_ABI32)
	{ .gettime = libc_clock_gettime, .send = libc_mq_timedsend, .receive = libc_mq_timedreceive, .type = TST_LIBC_TIMESPEC, .desc = "vDSO or syscall with libc spec"},
	{ .gettime = sys_clock_gettime, .send = sys_mq_timedsend, .receive = sys_mq_timedreceive, .type = TST_LIBC_TIMESPEC, .desc = "syscall with libc spec"},
	{ .gettime = sys_clock_gettime, .send = sys_mq_timedsend, .receive = sys_mq_timedreceive, .type = TST_KERN_OLD_TIMESPEC, .desc = "syscall with kernel spec32"},
#endif

#if defined(TST_ABI64)
	{ .gettime = sys_clock_gettime, .send = sys_mq_timedsend, .receive = sys_mq_timedreceive, .type = TST_KERN_TIMESPEC, .desc = "syscall with kernel spec64"},
#endif

#if (__NR_mq_timedsend_time64 != __LTP__NR_INVALID_SYSCALL)
	{ .gettime = sys_clock_gettime64, .send = sys_mq_timedsend64, .receive = sys_mq_timedreceive64, .type = TST_KERN_TIMESPEC, .desc = "syscall time64 with kernel spec64"},
#endif
};

static void setup(void)
{
	struct test_variants *tv = &variants[tst_variant];

	tst_res(TINFO, "Testing variant: %s", tv->desc);
	ts.type = tv->type;

	setup_common();
}

static void do_test(unsigned int i)
{
	struct test_variants *tv = &variants[tst_variant];
	const struct test_case *tc = &tcase[i];
	unsigned int j;
	unsigned int prio;
	size_t len = MAX_MSGSIZE;
	char rmsg[len];
	pid_t pid = -1;

	tst_ts_set_sec(&ts, tc->tv_sec);
	tst_ts_set_nsec(&ts, tc->tv_nsec);

	if (tc->signal)
		pid = set_sig(tc->rq, tv->gettime);

	if (tc->timeout)
		set_timeout(tc->rq, tv->gettime);

	if (tc->send) {
		if (tv->send(*tc->fd, smsg, tc->len, tc->prio, NULL) < 0) {
			tst_res(TFAIL | TTERRNO, "mq_timedsend() failed");
			return;
		}
	}

	if (tc->invalid_msg)
		len -= 1;

	TEST(tv->receive(*tc->fd, rmsg, len, &prio, tst_ts_get(tc->rq)));

	if (pid > 0)
		kill_pid(pid);

	if (*tc->fd == fd)
		cleanup_queue(fd);

	if (TST_RET < 0) {
		if (tc->err != TST_ERR)
			tst_res(TFAIL | TTERRNO,
				"mq_timedreceive() failed unexpectedly, expected %s",
				tst_strerrno(tc->err));
		else
			tst_res(TPASS | TTERRNO, "mq_timedreceive() failed expectedly");

		return;
	}

	if (tc->len != TST_RET) {
		tst_res(TFAIL, "mq_timedreceive() wrong length %ld, expected %u",
			TST_RET, tc->len);
		return;
	}

	if (tc->prio != prio) {
		tst_res(TFAIL, "mq_timedreceive() wrong prio %d, expected %d",
			prio, tc->prio);
		return;
	}

	for (j = 0; j < tc->len; j++) {
		if (rmsg[j] != smsg[j]) {
			tst_res(TFAIL,
				"mq_timedreceive() wrong data %d in %u, expected %d",
				rmsg[j], i, smsg[j]);
			return;
		}
	}

	tst_res(TPASS, "mq_timedreceive() returned %ld, priority %u, length: %zu",
			TST_RET, prio, len);
}

static struct tst_test test = {
	.tcnt = ARRAY_SIZE(tcase),
	.test = do_test,
	.test_variants = ARRAY_SIZE(variants),
	.setup = setup,
	.cleanup = cleanup_common,
	.forks_child = 1,
};
