// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines  Corp., 2001
 *  03/2001 - Written by Wayne Boyer
 */

/*\
 * [Description]
 *
 * Test for ENOSPC error.
 *
 * ENOSPC -  All possible shared memory segments have been taken (SHMMNI).
 */
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <stdlib.h>
#include <pwd.h>
#include <sys/shm.h>
#include "tst_test.h"
#include "tst_safe_sysv_ipc.h"
#include "libnewipc.h"

static int *segments;
static int maxshms, segments_cnt;
static key_t shmkey;

static void verify_shmget(void)
{
	int res = 0, num;

	errno = 0;
	for (num = 0; num <= maxshms; ++num) {
		res = shmget(shmkey + num, SHM_SIZE, IPC_CREAT | IPC_EXCL | SHM_RW);
		if (res == -1)
			break;
		segments[segments_cnt++] = res;
	}

	if (res != -1 || errno != ENOSPC)
		tst_brk(TFAIL | TERRNO, "Failed to trigger ENOSPC error");

	tst_res(TPASS, "Maximum number of segments reached (%d), used by test %d",
		maxshms, segments_cnt);
}

static void setup(void)
{
	shmkey = GETIPCKEY();

	SAFE_FILE_SCANF("/proc/sys/kernel/shmmni", "%i", &maxshms);

	segments = SAFE_MALLOC((maxshms + 1) * sizeof(int));
}

static void cleanup(void)
{
	int num;

	if (!segments)
		return;

	for (num = 0; num < segments_cnt; num++)
		SAFE_SHMCTL(segments[num], IPC_RMID, NULL);

	free(segments);
}

static struct tst_test test = {
	.needs_tmpdir = 1,
	.setup = setup,
	.cleanup = cleanup,
	.test_all = verify_shmget,
};
