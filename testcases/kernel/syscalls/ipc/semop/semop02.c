// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright (c) International Business Machines  Corp., 2001 */

/*
 * DESCRIPTION
 *	semop02 - test for E2BIG, EACCES, EFAULT, EINVAL and ERANGE errors
 *
 * HISTORY
 *	03/2001 - Written by Wayne Boyer
 *
 *      10/03/2008 Renaud Lottiaux (Renaud.Lottiaux@kerlabs.com)
 *      - Fix concurrency issue. The second key used for this test could
 *        conflict with the key from another task.
 */

#define _GNU_SOURCE
#include <pwd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include "tst_test.h"
#include "libnewipc.h"
#include "lapi/semun.h"
#include "semop.h"

static int sem_id_1 = -1;	/* a semaphore set with read & alter permissions */
static int sem_id_2 = -1;	/* a semaphore set without read & alter permissions */
static int bad_id = -1;
static key_t semkey;
static struct sembuf s_buf[PSEMS];

#define NSOPS	5		/* a resonable number of operations */
#define	BIGOPS	1024		/* a value that is too large for the number */
				/* of semop operations that are permitted   */
static struct test_case_t {
	int *semid;
	struct sembuf *t_sbuf;
	unsigned t_ops;
	int error;
} tc[] = {
	{&sem_id_1, (struct sembuf *)&s_buf, BIGOPS, E2BIG},
	{&sem_id_2, (struct sembuf *)&s_buf, NSOPS, EACCES},
	{&sem_id_1, (struct sembuf *)-1, NSOPS, EFAULT},
	{&sem_id_1, (struct sembuf *)&s_buf, 0, EINVAL},
	{&bad_id, (struct sembuf *)&s_buf, NSOPS, EINVAL},
	{&sem_id_1, (struct sembuf *)&s_buf, 1, ERANGE}
};

static void setup(void)
{
	char nobody_uid[] = "nobody";
	struct passwd *ltpuser;
	key_t semkey2;
	struct seminfo ipc_buf;
	union semun arr;

	tst_res(TINFO, "Testing variant: %s", variants[tst_variant].desc);

	ltpuser = SAFE_GETPWNAM(nobody_uid);
	SAFE_SETUID(ltpuser->pw_uid);

	/* get an IPC resource key */
	semkey = GETIPCKEY();

	/* create a semaphore set with read and alter permissions */
	sem_id_1 = semget(semkey, PSEMS, IPC_CREAT | IPC_EXCL | SEM_RA);
	if (sem_id_1 == -1)
		tst_brk(TBROK | TERRNO, "couldn't create semaphore in setup");

	/* Get an new IPC resource key. */
	semkey2 = GETIPCKEY();

	/* create a semaphore set without read and alter permissions */
	sem_id_2 = semget(semkey2, PSEMS, IPC_CREAT | IPC_EXCL);
	if (sem_id_2 == -1)
		tst_brk(TBROK | TERRNO, "couldn't create semaphore in setup");

	arr.__buf = &ipc_buf;
	if (semctl(sem_id_1, 0, IPC_INFO, arr) == -1)
		tst_brk(TBROK | TERRNO, "semctl() IPC_INFO failed");

	/* for ERANGE errno test */
	arr.val = 1;
	s_buf[0].sem_op = ipc_buf.semvmx;
	if (semctl(sem_id_1, 0, SETVAL, arr) == -1)
		tst_brk(TBROK | TERRNO, "semctl() SETVAL failed");
}

static void run(unsigned int i)
{
	struct test_variants *tv = &variants[tst_variant];
	struct tst_ts timeout;

	timeout.type = tv->type;
	tst_ts_set_nsec(&timeout, 10000);

	TEST(call_semop(tv, *(tc[i].semid), tc[i].t_sbuf, tc[i].t_ops, &timeout));

	if (TST_RET != -1) {
		tst_res(TFAIL | TTERRNO, "call succeeded unexpectedly");
		return;
	}

	if (TST_ERR == tc[i].error) {
		tst_res(TPASS | TTERRNO, "semop failed as expected");
	} else {
		tst_res(TFAIL | TTERRNO,
			 "semop failed unexpectedly; expected: "
			 "%d - %s", tc[i].error, strerror(tc[i].error));
	}
}

static void cleanup(void)
{
	union semun arr;

	if (sem_id_1 != -1) {
		if (semctl(sem_id_1, 0, IPC_RMID, arr) == -1)
			tst_res(TINFO, "WARNING: semaphore deletion failed.");
	}
	if (sem_id_2 != -1) {
		if (semctl(sem_id_2, 0, IPC_RMID, arr) == -1)
			tst_res(TINFO, "WARNING: semaphore deletion failed.");
	}
}

static struct tst_test test = {
	.test = run,
	.tcnt = ARRAY_SIZE(tc),
	.test_variants = ARRAY_SIZE(variants),
	.setup = setup,
	.cleanup = cleanup,
	.needs_tmpdir = 1,
	.needs_root = 1,
};
