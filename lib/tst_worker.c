// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2022 SUSE LLC <rpalethorpe@suse.com>
 */

#define _GNU_SOURCE
#define TST_NO_DEFAULT_MAIN

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include "tst_test.h"
#include "tst_worker.h"

static struct tst_state_matrix worker_state_mat = {
	.names = {
		[WS_STOPPED] = "Stopped",
		[WS_RUNNING] = "Running",
		[WS_STOPPING] = "Stopping",
		[WS_KILL_SENT] = "Kill sent",
		[WS_DIED] = "Dead"
	},
	.states = {
		[WS_STOPPED] = 1 << WS_RUNNING,
		[WS_RUNNING] = 1 << WS_STOPPING | 1 << WS_STOPPED | 1 << WS_KILL_SENT | 1 << WS_DIED,
		[WS_STOPPING] = 1 << WS_STOPPED | 1 << WS_KILL_SENT | 1 << WS_DIED,
		[WS_KILL_SENT] = 1 << WS_STOPPED | 1 << WS_DIED,
		[WS_DIED] = 1 << WS_RUNNING,
	},
};

static char *idstr(struct tst_worker *self)
{
	if (self->display_buf[0] != '\0')
		return self->display_buf;

	snprintf(self->display_buf,
		 sizeof(self->display_buf) - 1,
		 "%s Worker %d (%d)", self->name, self->pid, self->i);

	self->display_buf[sizeof(self->display_buf) - 1] = '\0';

	return self->display_buf;
}

static void worker_chan_on_send(struct tst_chan *chan, char *sent, size_t len)
{
	struct tst_worker *self = chan->user_priv;

	if (self->on_sent)
		self->on_sent(self, sent, len);
}

static void worker_chan_on_recv(struct tst_chan *chan, char *recved, size_t len)
{
	struct tst_worker *self = chan->user_priv;

	if (self->on_recved)
		self->on_recved(self, recved, len);
}

char *tst_worker_idstr(struct tst_worker *self)
{
	return idstr(self);
}

void tst_worker_start(struct tst_worker *self)
{
	struct tst_evloop *evloop = NULL;
	int infd[2];
	int outfd[2];

	SAFE_PIPE2(infd, O_CLOEXEC);
	SAFE_PIPE2(outfd, O_CLOEXEC);

	self->chan.user_priv = self;
	self->chan.priv = &self->pipe_chan;
	tst_chan_seen(&self->chan);
	self->pid = SAFE_FORK();

	if (!self->pid) {
		self->pid = getpid();

		close(infd[0]);
		close(outfd[1]);
		tst_pchan_open(&self->chan, outfd[0], infd[1], NULL);

		if (!tst_worker_ttl(self))
			tst_res(TWARN, "Worker timeout is too short; restarts take >%lldus", tst_chan_elapsed(&self->chan));

		exit(self->run(self));
	}

	close(infd[1]);
	close(outfd[0]);

	self->chan.on_send = worker_chan_on_send;
	self->chan.on_recv = worker_chan_on_recv;

	if (self->mode == TST_WORKER_ASYNC)
		evloop = &self->group->evloop;

	tst_pchan_open(&self->chan, infd[0], outfd[1], evloop);

	tst_res(TINFO, "%s: Started", idstr(self));
	TST_STATE_SET(&self->mach, WS_RUNNING);
}

int tst_worker_ttl(struct tst_worker *self)
{
	long long t = self->group->timeout;

	return MAX(0LL, t - tst_chan_elapsed(&self->chan));
}

void tst_worker_kill(struct tst_worker *w)
{
	const enum tst_worker_state ws =
		TST_STATE_GET(&w->mach, 1 << WS_RUNNING | 1 << WS_STOPPING | 1 << WS_KILL_SENT);

	if (ws != WS_KILL_SENT) {
		if (TST_STATE_GET(&w->chan.mach, TST_STATE_ANY) != CHS_CLOSED)
			w->chan.ops->close(&w->chan);

		tst_chan_seen(&w->chan);

		SAFE_KILL(w->pid, SIGKILL);
		TST_STATE_SET(&w->mach, WS_KILL_SENT);
		return;
	}

	tst_res(TWARN, "%s: Timed out again after KILL signal sent", idstr(w));

	if (w->on_died)
		w->on_died(w);
}

static int workers_waitpid(struct tst_workers *self)
{
	struct tst_worker *w = self->vec;
	int i,  ws;
	const pid_t pid = waitpid(-1, &ws, WNOHANG);

	if (!pid || (pid == -1 && errno == ECHILD))
		return 0;

	if (pid == -1)
		tst_brk(TBROK | TERRNO, "waitpid(-1, &ws, WNOHANG)");

	for (i = 0; i < self->count; i++) {
		if (w[i].pid == pid)
			break;
	}

	if (i == self->count) {
		tst_res(TWARN, "Received SIGCHLD for untracked PID: %d", pid);

		if (WIFEXITED(ws))
			tst_res(TINFO, "PID: %d: Exited: %d", pid, WEXITSTATUS(ws));
		if (WIFSIGNALED(ws))
			tst_res(TINFO, "PID: %d: Killed: %s", pid, tst_strsig(WTERMSIG(ws)));
		if (WCOREDUMP(ws))
			tst_res(TINFO, "PID: %d: Core dumped", pid);
	}

	w += i;

	if (WIFSTOPPED(ws) || WIFCONTINUED(ws))
		return 1;

	if (TST_STATE_GET(&w->chan.mach, TST_STATE_ANY) != CHS_CLOSED)
		w->chan.ops->close(&w->chan);

	if (WIFEXITED(ws) && !WEXITSTATUS(ws)) {
		TST_STATE_SET(&w->mach, WS_STOPPED);

		if (w->on_stopped)
			w->on_stopped(w);
		else
			tst_res(TINFO, "%s: Stopped", idstr(w));
	} else {
		TST_STATE_SET(&w->mach, WS_DIED);

		if (w->on_died)
			w->on_died(w);
		else
			tst_brk(TBROK, "%s: Died", idstr(w));
	}

	return 1;
}

static int workers_on_signal(struct tst_evloop *self,
			     struct signalfd_siginfo *si)
{
	struct tst_workers *const workers = self->priv;

	if (si->ssi_signo != SIGCHLD) {
		tst_brk(TBROK,
			"Don't know how to handle signal %s",
			tst_strsig(si->ssi_signo));
		return 0;
	}

	while (workers_waitpid(workers))
		;

	return 1;
}

static int workers_on_cont(struct tst_evloop *self)
{
	struct tst_workers *const workers = self->priv;
	int i, stopped = 0;

	for (i = 0; i < workers->count; i++) {
		struct tst_worker *w = workers->vec + i;
		const enum tst_worker_state ws = TST_STATE_GET(&w->mach, TST_STATE_ANY);

		if (ws == WS_STOPPED || ws == WS_DIED) {
			stopped++;
			continue;
		}

		if (tst_worker_ttl(w))
			continue;

		if (w->on_timeout) {
			w->on_timeout(w);
		} else {
			tst_res(TINFO, "%s: Timedout", idstr(w));
			tst_worker_kill(w);
		}
	}

	if (stopped == workers->count)
		return 0;

	return 1;
}

void tst_workers_setup(struct tst_workers *self)
{
	int i;

	self->evloop.priv = self;
	self->evloop.on_cont = workers_on_cont;
	self->evloop.on_signal = workers_on_signal;

	tst_evloop_setup(&self->evloop);

	for (i = 0; i < self->count; i++) {
		struct tst_worker *w = self->vec + i;

		w->mach.mat = &worker_state_mat;
		TST_STATE_EXP(&w->mach, 1 << WS_STOPPED);

		w->i = i;
		w->group = self;
	}
}

void tst_workers_cleanup(struct tst_workers *self)
{
	int i;

	for (i = 0; i < self->count; i++) {
		struct tst_worker *w = self->vec + i;
		const enum tst_worker_state ws = TST_STATE_GET(&w->mach, TST_STATE_ANY);

		if (TST_STATE_GET(&w->chan.mach, 1 << CHS_CLOSED) != CHS_CLOSED)
			w->chan.ops->close(&w->chan);

		if (ws != WS_STOPPED) {
			if (ws != WS_KILL_SENT)
				SAFE_KILL(w->pid, SIGKILL);

			tst_res(TWARN, "%s: Still running", idstr(w));
		}
	}

	tst_evloop_cleanup(&self->evloop);
}
