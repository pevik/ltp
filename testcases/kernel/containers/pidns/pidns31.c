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
 * mqueue and send a mqueue message from the child. Then check if signal
 * notification contains si_pid of the child.
 */

#define _GNU_SOURCE 1
#include <signal.h>
#include <mqueue.h>
#include "tst_test.h"
#include "tst_safe_posix_ipc.h"
#include "lapi/namespaces_constants.h"

#define MQNAME "/LTP_PIDNS30_MQ"

struct notify_info {
	mqd_t mqd;
	pid_t pid;
};

static mqd_t mqd = -1;
static volatile int received;

static void remove_mqueue(mqd_t mqd)
{
	if (mqd != -1)
		SAFE_MQ_CLOSE(mqd);

	mq_unlink(MQNAME);
}

static void child_signal_handler(LTP_ATTRIBUTE_UNUSED int sig, siginfo_t *si, LTP_ATTRIBUTE_UNUSED void *unused)
{
	struct notify_info *info;

	received = 0;

	tst_res(TINFO, "Received signal %s from pid %d", tst_strsig(si->si_signo), si->si_pid);

	if (si->si_signo != SIGUSR1 || si->si_code != SI_MESGQ)
		return;

	info = (struct notify_info*)si->si_ptr;

	if (si->si_pid == info->pid)
		received++;
}

static int child_func(LTP_ATTRIBUTE_UNUSED void *arg)
{
	pid_t cpid, ppid;
	mqd_t mqd_child;

	cpid = getpid();
	ppid = getppid();

	if (cpid != 1 || ppid != 0) {
		tst_res(TFAIL, "got unexpected result of cpid=%d ppid=%d", cpid, ppid);
		return 0;
	}

	TST_CHECKPOINT_WAIT(0);

	mqd_child = SAFE_MQ_OPEN(MQNAME, O_WRONLY, 0, NULL);
	SAFE_MQ_SEND(mqd_child, "pippo", 5, 1);

	TST_CHECKPOINT_WAKE(0);

	return 0;
}

static void cleanup(void)
{
	remove_mqueue(mqd);
}

static void run(void)
{
	int cpid, status;
	struct sigaction sa;
	struct sigevent notif;
	struct notify_info info;

	remove_mqueue(mqd);

	cpid = ltp_clone_quick(CLONE_NEWPID | SIGCHLD, child_func, NULL);
	if (cpid < 0)
		tst_brk(TBROK | TERRNO, "clone failed");

	mqd = SAFE_MQ_OPEN(MQNAME, O_RDWR | O_CREAT | O_EXCL, 0777, NULL);

	notif.sigev_notify = SIGEV_SIGNAL;
	notif.sigev_signo = SIGUSR1;
	info.mqd = mqd;
	info.pid = cpid;
	notif.sigev_value.sival_ptr = &info;

	SAFE_MQ_NOTIFY(mqd, &notif);

	sa.sa_flags = SA_SIGINFO;
	SAFE_SIGEMPTYSET(&sa.sa_mask);
	sa.sa_sigaction = child_signal_handler;
	SAFE_SIGACTION(SIGUSR1, &sa, NULL);

	TST_CHECKPOINT_WAKE_AND_WAIT(0);

	SAFE_WAITPID(cpid, &status, 0);

	if (received != 1) {
		tst_res(TFAIL, "Signal hasn't been received after mqueue event");
		return;
	}

	tst_res(TPASS, "Signal has been received after mqueue event");
}

static struct tst_test test = {
	.test_all = run,
	.cleanup = cleanup,
	.needs_root = 1,
	.needs_checkpoints = 1,
};
