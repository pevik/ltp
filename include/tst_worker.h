// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2022 SUSE LLC <rpalethorpe@suse.com>
 */

#include "tst_channel.h"

#ifndef TST_WORKER_H
#define TST_WORKER_H

enum tst_worker_mode {
	TST_WORKER_SYNC,
	TST_WORKER_ASYNC
};

enum tst_worker_state {
	WS_STOPPED,
	WS_RUNNING,
	WS_STOPPING,
	WS_KILL_SENT,
	WS_DIED
};

struct tst_workers;
struct tst_worker {
	int i;
	pid_t pid;
	struct tst_chan chan;
	struct tst_pchan pipe_chan;
	struct tst_workers *group;
	enum tst_worker_mode mode;
	struct tst_state_mach mach;

	char display_buf[128];
	char *name;

	void *priv;
	int (*run)(struct tst_worker *self);
	void (*on_stopped)(struct tst_worker *self);
	void (*on_died)(struct tst_worker *self);
	void (*on_timeout)(struct tst_worker *self);
	void (*on_sent)(struct tst_worker *self, char *sent, size_t len);
	void (*on_recved)(struct tst_worker *self, char *recv, size_t len);
};

struct tst_workers {
	long long timeout;
	struct tst_evloop evloop;

	long count;
	struct tst_worker *vec;
};

void tst_workers_setup(struct tst_workers *self);
void tst_workers_cleanup(struct tst_workers *self);

void tst_worker_start(struct tst_worker *self);
int tst_worker_ttl(struct tst_worker *self);
void tst_worker_kill(struct tst_worker *self);
char *tst_worker_idstr(struct tst_worker *self);

#endif
