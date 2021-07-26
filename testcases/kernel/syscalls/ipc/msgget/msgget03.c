// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines Corp., 2001
 */

/*
 * DESCRIPTION
 * test for an ENOSPC error by using up all available
 * message queues.
 *
 */

#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdlib.h>

#include "tst_test.h"
#include "tst_safe_sysv_ipc.h"
#include "libnewipc.h"

static int maxmsgs, queue_cnt;
static int *queues;
static key_t msgkey;

static void verify_msgget(void)
{
	int res = 0, num;

	errno = 0;
	for (num = 0; num <= maxmsgs; ++num) {
		res = msgget(msgkey + num, IPC_CREAT | IPC_EXCL);
		if (res == -1)
			break;
		queues[queue_cnt++] = res;
	}

	if (res != -1 || errno != ENOSPC)
		tst_brk(TFAIL | TERRNO, "Failed to trigger ENOSPC error");

	tst_res(TPASS, "Maximum number of queues reached (%d), used by test %d",
		maxmsgs, queue_cnt);
}

static void setup(void)
{
	msgkey = GETIPCKEY();

	SAFE_FILE_SCANF("/proc/sys/kernel/msgmni", "%i", &maxmsgs);

	queues = SAFE_MALLOC((maxmsgs + 1) * sizeof(int));
}

static void cleanup(void)
{
	int num;

	if (!queues)
		return;

	for (num = 0; num < queue_cnt; num++)
		SAFE_MSGCTL(queues[num], IPC_RMID, NULL);

	free(queues);
}

static struct tst_test test = {
	.needs_tmpdir = 1,
	.setup = setup,
	.cleanup = cleanup,
	.test_all = verify_msgget
};
