// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) Bull S.A.S. 2008
 *               01/12/08  Nadia Derbey <Nadia.Derbey@bull.net>
 * Copyright (C) 2022 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Clone a process with CLONE_NEWPID flag, register notification on a posix
 * mqueue and send a mqueue message from the parent. Then check if signal
 * notification contains si_pid of the parent.
 */

#define _GNU_SOURCE 1
#include <signal.h>
#include <mqueue.h>
#include "tst_test.h"
#include "lapi/namespaces_constants.h"

#define MQNAME "/LTP_PIDNS30_MQ"

static mqd_t mqd = -1;
static int received;

static void remove_mqueue(mqd_t mqd)
{
	if (mqd != -1)
		mq_close(mqd);

	mq_unlink(MQNAME);
}

static void child_signal_handler(LTP_ATTRIBUTE_UNUSED int sig, siginfo_t *si, LTP_ATTRIBUTE_UNUSED void *unused)
{
	if (si->si_signo != SIGUSR1 || si->si_code != SI_MESGQ || si->si_pid != 0)
		return;

	received++;
}

static int child_func(LTP_ATTRIBUTE_UNUSED void *arg)
{
	int ret;
	pid_t cpid, ppid;
	struct sigaction sa;
	struct sigevent notif;
	mqd_t mqd_child;

	cpid = getpid();
	ppid = getppid();

	if (cpid != 1 || ppid != 0) {
		tst_res(TFAIL, "got unexpected result of cpid=%d ppid=%d", cpid, ppid);
		return 0;
	}

	TST_CHECKPOINT_WAIT(0);

	mqd_child = mq_open(MQNAME, O_RDONLY, 0, NULL);
	if (mqd_child == -1)
		tst_brk(TBROK | TERRNO, "mq_open failed");

	notif.sigev_notify = SIGEV_SIGNAL;
	notif.sigev_signo = SIGUSR1;
	notif.sigev_value.sival_int = mqd_child;

	ret = mq_notify(mqd_child, &notif);
	if (ret == -1)
		tst_brk(TBROK | TERRNO, "mq_notify failed");

	sa.sa_flags = SA_SIGINFO;
	SAFE_SIGEMPTYSET(&sa.sa_mask);
	sa.sa_sigaction = child_signal_handler;
	SAFE_SIGACTION(SIGUSR1, &sa, NULL);

	TST_CHECKPOINT_WAKE_AND_WAIT(0);

	if (received != 1) {
		tst_res(TFAIL, "Signal hasn't been received after mqueue event");
		return 0;
	}

	tst_res(TPASS, "Signal has been received after mqueue event");

	return 0;
}

static void cleanup(void)
{
	remove_mqueue(mqd);
}

static void run(void)
{
	int ret;

	remove_mqueue(mqd);

	ret = ltp_clone_quick(CLONE_NEWPID | SIGCHLD, child_func, NULL);
	if (ret < 0)
		tst_brk(TBROK | TERRNO, "clone failed");

	mqd = mq_open(MQNAME, O_RDWR | O_CREAT | O_EXCL, 0777, 0);
	if (mqd == -1)
		tst_brk(TBROK | TERRNO, "mq_open failed");

	TST_CHECKPOINT_WAKE_AND_WAIT(0);

	ret = mq_send(mqd, "pippo", 5, 1);
	if (ret == -1)
		tst_brk(TBROK | TERRNO, "mq_send failed");

	TST_CHECKPOINT_WAKE(0);
}

static struct tst_test test = {
	.test_all = run,
	.cleanup = cleanup,
	.needs_root = 1,
	.needs_checkpoints = 1,
};
