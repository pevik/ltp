// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2024 FUJITSU LIMITED. All Rights Reserved.
 * Author: Yang Xu <xuyang2018.jy@fujitsu.com>
 */

/*\
 * [Description]
 *
 * Verify that flock(2) fails with
 *
 * - EINTR when waiting lock, call is interrupted by signal
 * - EWOULDBLOCK when file is locked and LOCK_NB flag is selected
 */

#include <signal.h>
#include <sys/file.h>
#include <sys/wait.h>
#include "tst_test.h"

#define TEST_INTR "test_intr"
#define TEST_EWOULDBLOCK "test_ewouldblock"

static struct test_case_t {
	char *filename;
	int expected_errno;
	int child;
	char *desc;
} tcases[] = {
	{TEST_INTR, EINTR, 1,
		"while waiting lock, call is interrupted by signal"},
	{TEST_EWOULDBLOCK, EWOULDBLOCK, 0,
		"file is locked and LOCK_NB flag is selected"},
};

static void setup(void)
{
	SAFE_TOUCH(TEST_INTR, 0777, NULL);
	SAFE_TOUCH(TEST_EWOULDBLOCK, 0777, NULL);
}

static void handler(int sig)
{
	switch (sig) {
	case SIGUSR1:
		tst_res(TINFO, "%s", "Got SIGUSR1");
	break;
	default:
		tst_res(TINFO, "%s", "Got other signal");
	break;
	}
}

static void child_do(int fd, struct test_case_t *tc)
{
	struct sigaction sa;

	sa.sa_handler = handler;
	SAFE_SIGEMPTYSET(&sa.sa_mask);
	SAFE_SIGACTION(SIGUSR1, &sa, NULL);

	TST_EXP_FAIL(flock(fd, LOCK_EX), tc->expected_errno, "%s", tc->desc);
}

static void verify_flock(unsigned int i)
{
	struct test_case_t *tc = &tcases[i];
	pid_t pid;
	int fd1 = SAFE_OPEN(tc->filename, O_RDWR);
	int fd2 = SAFE_OPEN(tc->filename, O_RDWR);

	if (tc->child) {
		flock(fd1, LOCK_EX);
		pid = SAFE_FORK();
		if (!pid) {
			child_do(fd2, tc);
			exit(0);
		}
		sleep(1);
		SAFE_KILL(pid, SIGUSR1);
		SAFE_WAITPID(pid, NULL, 0);
	} else {
		flock(fd1, LOCK_EX);
		TST_EXP_FAIL(flock(fd2, LOCK_EX | LOCK_NB), tc->expected_errno,
			"%s", tc->desc);
	}
	SAFE_CLOSE(fd1);
	SAFE_CLOSE(fd2);
}

static struct tst_test test = {
	.setup = setup,
	.tcnt = ARRAY_SIZE(tcases),
	.test = verify_flock,
	.needs_tmpdir = 1,
	.needs_root = 1,
	.forks_child = 1,
};
