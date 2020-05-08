// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright (c) International Business Machines  Corp., 2001 */

/*
 * NAME
 *	semop04.c
 *
 * DESCRIPTION
 *	semop04 - test for EAGAIN error
 *
 * ALGORITHM
 *	create a semaphore set with read and alter permissions
 *	loop if that option was specified
 *	call semop() with two different invalid cases
 *	check the errno value
 *	  issue a PASS message if we get EAGAIN
 *	otherwise, the tests fails
 *	  issue a FAIL message
 *	call cleanup
 *
 * USAGE:  <for command-line>
 *  semop04 [-c n] [-e] [-i n] [-I x] [-P x] [-t]
 *     where,  -c n : Run n copies concurrently.
 *             -e   : Turn on errno logging.
 *	       -i n : Execute test n times.
 *	       -I x : Execute test for x seconds.
 *	       -P x : Pause for x seconds between iterations.
 *	       -t   : Turn on syscall timing.
 *
 * HISTORY
 *	03/2001 - Written by Wayne Boyer
 *
 * RESTRICTIONS
 *	none
 */

#include <sys/sem.h>
#include "tst_test.h"
#include "libnewipc.h"
#include "lapi/semun.h"

static int sem_id = -1;
static int val; /* value for SETVAL */

static key_t semkey;
static struct sembuf s_buf;

static struct test_case_t {
	union semun get_arr;
	short op;
	short flg;
	short num;
	int error;
} tc[] = {
	/* EAGAIN sem_op = 0 */
	{ {
	1}, 0, IPC_NOWAIT, 2, EAGAIN},
	    /* EAGAIN sem_op = -1 */
	{ {
	0}, -1, IPC_NOWAIT, 2, EAGAIN}
};

static void run(unsigned int i)
{
	/* initialize the s_buf buffer */
	s_buf.sem_op = tc[i].op;
	s_buf.sem_flg = tc[i].flg;
	s_buf.sem_num = tc[i].num;

	/* initialize all the primitive semaphores */
	tc[i].get_arr.val = val--;
	if (semctl(sem_id, tc[i].num, SETVAL, tc[i].get_arr) == -1)
		tst_brk(TBROK | TERRNO, "semctl() failed");

	TEST(semop(sem_id, &s_buf, 1));
	if (TST_RET != -1) {
		tst_res(TFAIL, "call succeeded unexpectedly");
		return;
	}

	if (TST_ERR == tc[i].error)
		tst_res(TPASS | TTERRNO, "expected failure");
	else
		tst_res(TFAIL | TTERRNO, "unexpected failure");
}

static void setup(void)
{
	val = 1;

	/* get an IPC resource key */
	semkey = GETIPCKEY();

	/*
	 * create a semaphore set with read and alter permissions and PSEMS
	 * "primitive" semaphores.
	 */
	if ((sem_id = semget(semkey, PSEMS, IPC_CREAT | IPC_EXCL | SEM_RA)) ==
	     -1) {
		tst_brk(TBROK | TERRNO, "couldn't create semaphore in setup");
	}
}

static void cleanup(void)
{
	union semun arr;

	if (sem_id != -1) {
		if (semctl(sem_id, 0, IPC_RMID, arr) == -1)
			tst_res(TINFO, "WARNING: semaphore deletion failed.");
	}
}

static struct tst_test test = {
	.test = run,
	.tcnt = ARRAY_SIZE(tc),
	.setup = setup,
	.cleanup = cleanup,
	.needs_tmpdir = 1,
};
