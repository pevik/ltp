// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2017 Petr Vorel <pvorel@suse.cz>
 */

#ifndef MQ_TIMED_H
#define MQ_TIMED_H

#include "mq.h"
#include "tst_timer.h"

struct test_case {
	int *fd;
	unsigned int len;
	unsigned int prio;
	struct tst_ts *rq;
	long tv_sec;
	long tv_nsec;
	int invalid_msg;
	int send;
	int signal;
	int timeout;
	int ret;
	int err;
};

static pid_t set_sig(struct tst_ts *ts,
		     int (*gettime)(clockid_t clk_id, void *ts))
{
	gettime(CLOCK_REALTIME, tst_ts_get(ts));
	*ts = tst_ts_add_us(*ts, 3000000);

	return create_sig_proc(SIGINT, 40, 200000);
}

static void set_timeout(struct tst_ts *ts,
			int (*gettime)(clockid_t clk_id, void *ts))
{
	gettime(CLOCK_REALTIME, tst_ts_get(ts));
	*ts = tst_ts_add_us(*ts, 50000);
}

static void kill_pid(pid_t pid)
{
	SAFE_KILL(pid, SIGTERM);
	SAFE_WAIT(NULL);
}

#endif /* MQ_TIMED_H */
