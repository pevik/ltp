// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2022 SUSE LLC <rpalethorpe@suse.com>
 */

#include "inttypes.h"
#include "sys/signalfd.h"

#include "tst_epoll.h"

#ifndef TST_EVLOOP_H
#define TST_EVLOOP_H

struct tst_evloop {
	int epollfd;
	int signalfd;
	struct tst_epoll_event_data signalfd_evdata;
	int timeout;

	void *priv;
	int (*on_cont)(struct tst_evloop *self);
	int (*on_signal)(struct tst_evloop *self, struct signalfd_siginfo *si);
};

void tst_evloop_setup(struct tst_evloop *self);
void tst_evloop_run(struct tst_evloop *self);
void tst_evloop_add(struct tst_evloop *self,
		    struct tst_epoll_event_data *evdata,
		    int fd, uint32_t events);
void tst_evloop_cleanup(struct tst_evloop *self);

#endif
