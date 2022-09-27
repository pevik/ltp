// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2022 SUSE LLC <rpalethorpe@suse.com>
 */
#define _GNU_SOURCE
#define TST_NO_DEFAULT_MAIN

#include "tst_test.h"
#include "tst_evloop.h"

static void handle_epoll_event(struct epoll_event *event)
{
	struct tst_epoll_event_data *data = event->data.ptr;

	data->on_epoll(data->self, event->events);
}

static int evloop_on_signal(struct tst_evloop *self, uint32_t events)
{
	int i, n;
	struct signalfd_siginfo si[16];

	if (events ^ EPOLLIN) {
		tst_brk(TBROK, "Unexpected event on signalfd");
		return 1;
	}

	n = SAFE_READ(0, self->signalfd, si, sizeof(si));

	if (!n)
		tst_brk(TBROK, "Got EPOLLIN on signalfd, but no signal read from fd");

	for (i = 0; i < n/(int)sizeof(si[0]); i++) {
		if (!self->on_signal(self, si + i))
			return 0;
	}

	return 1;
}

void tst_evloop_add(struct tst_evloop *self,
		       struct tst_epoll_event_data *evdata,
		       int fd, uint32_t events)
{
	struct epoll_event ev = {
		.events = events,
		.data.ptr = evdata,
	};

	SAFE_EPOLL_CTL(self->epollfd, EPOLL_CTL_ADD, fd, &ev);
}

void tst_evloop_setup(struct tst_evloop *self)
{

	sigset_t mask;

	self->epollfd = SAFE_EPOLL_CREATE1(EPOLL_CLOEXEC);

	sigfillset(&mask);
	SAFE_SIGPROCMASK(SIG_BLOCK, &mask, NULL);
	self->signalfd = signalfd(-1, &mask, SFD_CLOEXEC);

	self->signalfd_evdata.self = self;
	self->signalfd_evdata.on_epoll = (tst_on_epoll_fn)evloop_on_signal;

	tst_evloop_add(self, &self->signalfd_evdata, self->signalfd, EPOLLIN);
}

void tst_evloop_run(struct tst_evloop *self)
{
	static int saturated_warn;
	const int maxevents = 128;
	struct epoll_event events[maxevents];

	for (;;) {
		const int ev_num = SAFE_EPOLL_WAIT(self->epollfd, events,
						   maxevents, self->timeout);

		for (int i = 0; i < ev_num; i++)
			handle_epoll_event(events + i);

		if (ev_num == maxevents) {
			if (!saturated_warn)
				tst_res(TINFO, "Event loop saturated");

			saturated_warn = 1;
			continue;
		}

		if (!self->on_cont(self))
			break;
	}
}

void tst_evloop_cleanup(struct tst_evloop *self)
{
	if (self->epollfd > 0)
		SAFE_CLOSE(self->epollfd);
	if (self->signalfd > 0)
		SAFE_CLOSE(self->signalfd);
}
