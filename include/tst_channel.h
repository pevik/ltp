// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2022 SUSE LLC <rpalethorpe@suse.com>
 */

#include "tst_evloop.h"
#include "tst_state_machine.h"

#ifndef TST_CHANNEL_H
#define TST_CHANNEL_H

enum tst_chan_mode {
	CHM_SYNC,
	CHM_ASYNC,
};

enum tst_chan_state {
	CHS_CLOSED,
	CHS_READY,
	CHS_RECV,
	CHS_SEND
};

struct tst_chan_buf {
	char *ptr;
	size_t len;
	size_t off;
};

struct tst_chan;
struct tst_chan_ops {
	void (*const close)(struct tst_chan *self);

	void (*const send)(struct tst_chan *self);
	void (*const recv)(struct tst_chan *self);

	int (*const on_epoll)(struct tst_chan *self, uint32_t events);
};

struct tst_chan {
	const struct tst_chan_ops *ops;
	void *priv;

	enum tst_chan_mode mode;
	struct tst_state_mach mach;
	long long last_seen;

	struct tst_epoll_event_data evdata;

	struct tst_chan_buf in;
	struct tst_chan_buf out;

	void *user_priv;
	void (*on_send)(struct tst_chan *self, char *sent, size_t len);
	void (*on_recv)(struct tst_chan *self, char *recv, size_t len);
};

enum tst_pchan_msg_kind {
	PCMK_ACK = 1,
	PCMK_DATA
};

struct tst_pchan_envelope {
	unsigned int kind;
	unsigned int len;
} __attribute__((packed));

enum tst_pchan_state {
	PCS_IDLE,
	PCS_RECV_DATA,
	PCS_SEND_ACK,
	PCS_SEND_DATA,
	PCS_RECV_ACK,
};

struct tst_pchan {
	int infd;
	int outfd;

	struct tst_pchan_envelope envelope;
	struct tst_chan_buf envelope_buf;

	unsigned int out_full:1;

	struct tst_state_mach mach;
};

void tst_chan_send(struct tst_chan *self, char *msg, size_t len);
void tst_chan_recv(struct tst_chan *self, char *msg, size_t len);
void tst_chan_seen(struct tst_chan *self);
long long tst_chan_elapsed(struct tst_chan *self);

void tst_pchan_open(struct tst_chan *self, int infd, int outfd,
		    struct tst_evloop *const evloop);
void tst_pchan_close(struct tst_chan *self);

#endif
