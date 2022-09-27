// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2022 SUSE LLC <rpalethorpe@suse.com>
 */
#define _GNU_SOURCE
#define TST_NO_DEFAULT_MAIN

#include "tst_timer.h"
#include "tst_safe_clocks.h"
#include "tst_channel.h"

static struct tst_state_matrix chan_state_mat = {
	.names = {
		[CHS_CLOSED] = "closed",
		[CHS_READY] = "ready",
		[CHS_RECV] = "receiving",
		[CHS_SEND] = "sending"
	},
	.states = {
		[CHS_CLOSED] = 1 << CHS_READY,
		[CHS_READY] = 1 << CHS_CLOSED | 1 << CHS_RECV | 1 << CHS_SEND,
		[CHS_RECV] = 1 << CHS_CLOSED | 1 << CHS_READY,
		[CHS_SEND] = 1 << CHS_CLOSED | 1 << CHS_READY,
	}
};

static struct tst_state_matrix pchan_state_mat = {
	.names = {
		[PCS_IDLE] = "idle",
		[PCS_RECV_DATA] = "receiving data",
		[PCS_SEND_ACK] = "sending ack",
		[PCS_SEND_DATA]= "sending data",
		[PCS_RECV_ACK] = "receving ack",
	},
	.states = {
		[PCS_IDLE] = 1 << PCS_IDLE | 1 << PCS_RECV_DATA | 1 << PCS_SEND_DATA,
		[PCS_RECV_DATA] = 1 << PCS_SEND_ACK | 1 << PCS_IDLE,
		[PCS_SEND_ACK] = 1 << PCS_IDLE,
		[PCS_SEND_DATA]= 1 << PCS_RECV_ACK | 1 << PCS_IDLE,
		[PCS_RECV_ACK] = 1 << PCS_IDLE,
	},
};

static size_t chan_buf_more(struct tst_chan_buf *self)
{
	return self->off < self->len;
}

void tst_chan_send(struct tst_chan *self, char *msg, size_t len)
{
	if (self->mode == CHM_SYNC)
		goto send;

	if (!self->on_send) {
		tst_brk(TBROK, "In async mode, but on_sent cb not set");
		return;
	}

	if (!self->evdata.on_epoll) {
		tst_brk(TBROK, "In async mode, but not added to epoll");
		return;
	}

send:
	self->out.ptr = msg;
	self->out.len = len;
	self->out.off = 0;

	TST_STATE_SET(&self->mach, CHS_SEND);
	self->ops->send(self);
}

void tst_chan_recv(struct tst_chan *self, char *msg, size_t len)
{
	if (self->mode == CHM_SYNC)
		goto recv;

	if (!self->on_recv) {
		tst_brk(TBROK, "In async mode, but on_recv cb not set");
		return;
	}

	if (!self->evdata.on_epoll) {
		tst_brk(TBROK, "In async mode, but not added to epoll");
		return;
	}

recv:
	self->in.ptr = msg;
	self->in.len = len;
	self->in.off = 0;

	TST_STATE_SET(&self->mach, CHS_RECV);
	self->ops->recv(self);
}

void tst_chan_seen(struct tst_chan *self)
{
	struct timespec now;

	SAFE_CLOCK_GETTIME(CLOCK_MONOTONIC_RAW, &now);
	self->last_seen = tst_timespec_to_us(now);
}

long long tst_chan_elapsed(struct tst_chan *self)
{
	struct timespec now;

	SAFE_CLOCK_GETTIME(CLOCK_MONOTONIC_RAW, &now);

	return tst_timespec_to_us(now) - self->last_seen;
}

static void pipe_chan_write(struct tst_chan *self, struct tst_chan_buf *buf)
{
	ssize_t ret;
	size_t written = buf->off;
	struct tst_pchan *priv = self->priv;

	while (written < buf->len) {
		ret = write(priv->outfd,
			    buf->ptr + written, buf->len - written);

		if (ret == -1) {
			if (self->mode == CHM_ASYNC && errno == EAGAIN) {
				priv->out_full = 1;
				break;
			}

			if (errno == EINTR)
				continue;

			tst_brk(TBROK | TERRNO, "write");
		}

		written += ret;
	}

	buf->off = written;
}

static void pipe_chan_read(struct tst_chan *self, struct tst_chan_buf *buf)
{
	ssize_t ret;
	size_t recved = buf->off;
	struct tst_pchan *priv = self->priv;

	while (recved < buf->len) {
		ret = read(priv->infd,
			   buf->ptr + recved, buf->len - recved);

		if (!ret)
			tst_brk(TBROK, "PID %d: read(%d) = EOF", getpid(), priv->infd);

		if (ret == -1) {
			if (self->mode == CHM_ASYNC && errno == EAGAIN)
				break;

			if (errno == EINTR)
				continue;

			tst_brk(TBROK | TERRNO, "read");
		}

		recved += ret;
	}

	buf->off = recved;
}

static void pipe_chan_send(struct tst_chan *self)
{
	struct tst_pchan *priv = self->priv;
	const enum tst_pchan_state state =
		TST_STATE_GET(&priv->mach, 1 << PCS_IDLE | 1 << PCS_SEND_DATA | 1 << PCS_RECV_ACK);

	TST_STATE_EXP(&self->mach, 1 << CHS_READY | 1 << CHS_SEND);

	switch (state) {
	case PCS_IDLE:
		priv->envelope.kind = PCMK_DATA;
		priv->envelope.len = self->out.len;
		priv->envelope_buf.ptr = (char *)&priv->envelope;
		priv->envelope_buf.len = sizeof(priv->envelope);
		priv->envelope_buf.off = 0;

		TST_STATE_SET(&priv->mach, PCS_SEND_DATA);
		break;
	case PCS_SEND_DATA:
		break;
	case PCS_RECV_ACK:
		goto ack;
	default:
		break;
	}

	if (priv->out_full)
		return;

	pipe_chan_write(self, &priv->envelope_buf);
	if (chan_buf_more(&priv->envelope_buf))
		return;

	pipe_chan_write(self, &self->out);
	if (chan_buf_more(&self->out))
		return;

	TST_STATE_SET(&priv->mach, PCS_RECV_ACK);
	priv->envelope_buf.off = 0;

	if (self->mode == CHM_ASYNC)
		return;
ack:
	pipe_chan_read(self, &priv->envelope_buf);

	if (chan_buf_more(&priv->envelope_buf))
		return;

	if (priv->envelope.kind != PCMK_ACK || priv->envelope.len) {
		tst_brk(TBROK, "Malformed ack");
		return;
	}

	tst_chan_seen(self);
	TST_STATE_SET(&priv->mach, PCS_IDLE);
	TST_STATE_SET(&self->mach, CHS_READY);

	if (self->on_send)
		self->on_send(self, self->out.ptr, self->out.len);
}

static void pipe_chan_recv(struct tst_chan *self)
{
	struct tst_pchan *priv = self->priv;
	const enum tst_pchan_state state =
		TST_STATE_GET(&priv->mach, 1 << PCS_IDLE | 1 << PCS_RECV_DATA | 1 << PCS_SEND_ACK);

	TST_STATE_EXP(&self->mach, 1 << CHS_READY | 1 << CHS_RECV);

	switch (state) {
	case PCS_IDLE:
		priv->envelope_buf.off = 0;

		TST_STATE_SET(&priv->mach, PCS_RECV_DATA);
		break;
	case PCS_RECV_DATA:
		break;
	case PCS_SEND_ACK:
		goto ack;
	default:
		break;
	}

	pipe_chan_read(self, &priv->envelope_buf);
	if (chan_buf_more(&priv->envelope_buf))
		return;

	if (priv->envelope.kind != PCMK_DATA) {
		tst_brk(TBROK, "Expected data message, but got: %d", priv->envelope.kind);
		return;
	}

	if (priv->envelope.len > self->in.len) {
		tst_brk(TBROK, "Incoming message too large: %ul", priv->envelope.len);
		return;
	}

	self->in.len = priv->envelope.len;
	pipe_chan_read(self, &self->in);
	if (chan_buf_more(&self->in))
		return;

	TST_STATE_SET(&priv->mach, PCS_SEND_ACK);
	priv->envelope.kind = PCMK_ACK;
	priv->envelope.len = 0;
	priv->envelope_buf.off = 0;

ack:
	pipe_chan_write(self, &priv->envelope_buf);
	if (chan_buf_more(&priv->envelope_buf))
		return;

	TST_STATE_SET(&priv->mach, PCS_IDLE);
	TST_STATE_SET(&self->mach, CHS_READY);

	if (self->on_recv)
		self->on_recv(self, self->in.ptr, self->in.len);
}

static int pipe_chan_on_epoll(struct tst_chan *self, uint32_t events)
{
	struct tst_pchan *priv = self->priv;
	enum tst_chan_state chs = TST_STATE_GET(&self->mach, TST_STATE_ANY);
	enum tst_pchan_state phs = TST_STATE_GET(&priv->mach, TST_STATE_ANY);

	if (events | EPOLLOUT) {
		priv->out_full = 0;

		switch (chs) {
		case CHS_RECV:
			if (phs == PCS_RECV_DATA)
				break;

			self->ops->recv(self);
			break;
		case CHS_SEND:
			if (phs == PCS_RECV_ACK)
				break;

			self->ops->send(self);
			break;
		case CHS_READY:
		case CHS_CLOSED:
			TST_STATE_EXP(&priv->mach, 1 << PCS_IDLE);
			break;
		}
	}

	if (events | EPOLLIN) {
		switch (chs) {
		case CHS_RECV:
			if (phs == PCS_SEND_ACK)
				break;

			self->ops->recv(self);
			break;
		case CHS_SEND:
			if (phs == PCS_SEND_DATA)
				break;

			self->ops->send(self);
			break;
		case CHS_READY:
		case CHS_CLOSED:
			TST_STATE_EXP(&priv->mach, 1 << PCS_IDLE);
			break;
		}
	}

	if (events & EPOLLERR) {
		switch (phs) {
		case PCS_RECV_DATA:
		case PCS_SEND_ACK:
		case PCS_SEND_DATA:
			tst_brk(TBROK, "Channel closed during operation");
		default:
			break;
		}

		if (chs != CHS_CLOSED)
			self->ops->close(self);
	}

	return 1;
}

static struct tst_chan_ops pipe_chan_ops = {
	.close = tst_pchan_close,
	.send = pipe_chan_send,
	.recv = pipe_chan_recv,
	.on_epoll = pipe_chan_on_epoll,
};

void tst_pchan_open(struct tst_chan *self,
		    int infd, int outfd,
		    struct tst_evloop *const evloop)
{
	struct tst_pchan *priv = self->priv;

	self->mach.mat = &chan_state_mat;
	TST_STATE_EXP(&self->mach, 1 << CHS_CLOSED);

	self->ops = &pipe_chan_ops;

	if (!priv)
		tst_brk(TBROK, "Channel should have pipe_chan priv preallocated");

	priv->mach.mat = &pchan_state_mat;
	TST_STATE_EXP(&priv->mach, 1 << PCS_IDLE);

	priv->out_full = 0;
	priv->infd = infd;
	priv->outfd = outfd;
	priv->envelope_buf.ptr = (char *)&priv->envelope;
	priv->envelope_buf.len = sizeof(priv->envelope);

	if (!evloop)
		goto out;

	self->mode = CHM_ASYNC;
	self->evdata.on_epoll = (tst_on_epoll_fn)self->ops->on_epoll;
	self->evdata.self = self;
	tst_evloop_add(evloop, &self->evdata, infd, EPOLLIN);
	tst_evloop_add(evloop, &self->evdata, outfd, EPOLLOUT | EPOLLET);

out:
	TST_STATE_SET(&self->mach, CHS_READY);
	tst_chan_seen(self);
}

void tst_pchan_close(struct tst_chan *self)
{
	struct tst_pchan *priv = self->priv;

	close(priv->infd);
	close(priv->outfd);

	TST_STATE_SET(&priv->mach, PCS_IDLE);
	TST_STATE_SET(&self->mach, CHS_CLOSED);
}
