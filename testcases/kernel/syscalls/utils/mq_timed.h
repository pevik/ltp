/*
 * Copyright (c) 2017 Petr Vorel <pvorel@suse.cz>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it would be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
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
