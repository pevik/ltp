// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright (c) International Business Machines  Corp., 2001 */

/*
 * NAME
 *	semop03.c
 *
 * DESCRIPTION
 *	semop03 - test for EFBIG error
 *
 * ALGORITHM
 *	create a semaphore set with read and alter permissions
 *	loop if that option was specified
 *	call semop() using two different invalid cases
 *	check the errno value
 *	  issue a PASS message if we get EFBIG
 *	otherwise, the tests fails
 *	  issue a FAIL message
 *	call cleanup
 *
 * USAGE:  <for command-line>
 *  semop03 [-c n] [-e] [-i n] [-I x] [-P x] [-t]
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
#include "semop.h"

static key_t semkey;
static int sem_id = -1;
static struct sembuf s_buf;

static int tc[] = { -1, PSEMS + 1 }; /* negative and too many "primitive" semas */

static void run(unsigned int i)
{
	struct test_variants *tv = &variants[tst_variant];
	struct tst_ts timeout;

	timeout.type = tv->type;
	tst_ts_set_nsec(&timeout, 10000);

	/* initialize two fields in the sembuf structure here */
	s_buf.sem_op = 1;	/* add this value to struct sem.semval */
	s_buf.sem_flg = SEM_UNDO;	/* undo when process exits */

	/*
	 * initialize the last field in the sembuf structure to the test
	 * dependent value.
	 */
	s_buf.sem_num = tc[i];

	/*
	 * use the TEST macro to make the call
	 */

	TEST(call_semop(tv, sem_id, &s_buf, 1, &timeout));

	if (TST_RET != -1) {
		tst_res(TFAIL | TTERRNO, "call succeeded unexpectedly");
		return;
	}

	switch (TST_ERR) {
	case EFBIG:
		tst_res(TPASS | TTERRNO, "expected failure");
		break;
	default:
		tst_res(TFAIL | TTERRNO, "unexpected failure");
		break;
	}
}

static void setup(void)
{
	tst_res(TINFO, "Testing variant: %s", variants[tst_variant].desc);

	/* get an IPC resource key */
	semkey = GETIPCKEY();

	/* create a semaphore with read and alter permissions */
	if ((sem_id = semget(semkey, PSEMS, IPC_CREAT | IPC_EXCL | SEM_RA)) ==
	    -1)
		tst_brk(TBROK | TERRNO, "couldn't create semaphore in setup");
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
	.test_variants = ARRAY_SIZE(variants),
	.setup = setup,
	.cleanup = cleanup,
	.needs_tmpdir = 1,
};
