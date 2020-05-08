// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright (c) International Business Machines  Corp., 2001 */

/*
 * NAME
 *	semop01.c
 *
 * DESCRIPTION
 *	semop01 - test that semop() basic functionality is correct
 *
 * ALGORITHM
 *	create a semaphore set and initialize some values
 *	loop if that option was specified
 *	call semop() to set values for the primitive semaphores
 *	check the return code
 *	  if failure, issue a FAIL message.
 *	otherwise,
 *	  if doing functionality testing
 *		get the semaphore values and compare with expected values
 *		if correct,
 *			issue a PASS message
 *		otherwise
 *			issue a FAIL message
 *	  else issue a PASS message
 *	call cleanup
 *
 * HISTORY
 *	03/2001  - Written by Wayne Boyer
 *	17/01/02 - Modified. Manoj Iyer, IBM Austin. TX. manjo@austin.ibm.com
 *	           4th argument to semctl() system call was modified according
 *	           to man pages.
 *	           In my opinion The test should not even have compiled but
 *	           it was working due to some mysterious reason.
 *
 * RESTRICTIONS
 *	none
 */

#include <stdlib.h>
#include <sys/sem.h>
#include "tst_test.h"
#include "libnewipc.h"
#include "lapi/semun.h"

#define NSEMS	4		/* the number of primitive semaphores to test */

static int sem_id = -1;		/* a semaphore set with read & alter permissions */
static key_t semkey;

static union semun get_arr;
static struct sembuf sops[PSEMS];

static void run(void)
{
	union semun arr = { .val = 0 };
	int fail = 0;
	int i;

	TEST(semop(sem_id, sops, NSEMS));

	if (TST_RET == -1) {
		tst_res(TFAIL | TTERRNO, "semop() failed");
	} else {
		/*
		 * Get the values and make sure they are the same as what was
		 * set
		 */
		if (semctl(sem_id, 0, GETALL, get_arr) == -1) {
			tst_brk(TBROK | TERRNO, "semctl() failed in functional test");
		}

		for (i = 0; i < NSEMS; i++) {
			if (get_arr.array[i] != i * i) {
				fail = 1;
			}
		}
		if (fail)
			tst_res(TFAIL | TERRNO, "semaphore values are wrong");
		else
			tst_res(TPASS, "semaphore values are correct");
	}

	/*
	 * clean up things in case we are looping
	 */
	for (i = 0; i < NSEMS; i++) {
		if (semctl(sem_id, i, SETVAL, arr) == -1) {
			tst_brk(TBROK | TERRNO, "semctl failed");
		}
	}
}

static void setup(void)
{
	int i;

	get_arr.array = malloc(sizeof(unsigned short int) * PSEMS);
	if (get_arr.array == NULL)
		tst_brk(TBROK, "malloc failed");

	semkey = GETIPCKEY();

	sem_id = semget(semkey, PSEMS, IPC_CREAT | IPC_EXCL | SEM_RA);
	if (sem_id == -1)
		tst_brk(TBROK | TERRNO, "couldn't create semaphore in setup");

	for (i = 0; i < NSEMS; i++) {
		sops[i].sem_num = i;
		sops[i].sem_op = i * i;
		sops[i].sem_flg = SEM_UNDO;
	}
}

static void cleanup(void)
{
	union semun arr;

	if (sem_id != -1) {
		if (semctl(sem_id, 0, IPC_RMID, arr) == -1)
			tst_res(TINFO, "WARNING: semaphore deletion failed.");
	}
	free(get_arr.array);
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.cleanup = cleanup,
	.needs_tmpdir = 1,
};
