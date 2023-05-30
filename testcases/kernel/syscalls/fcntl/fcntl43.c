/*
 * [Description]
 * Tests gfs2 dlm posix op queue handling in the kernel.
 * It is recommended to run watch -n 0.1 "dlm_tool plocks $LS"
 * aside to monitor dlm plock handling.
 *
 * This testcase creates two lock requests which looks identical and should
 * be granted at the same time.
 *
 * [How to use it]
 * Call it with TMPDIR=/mnt ./fcntl43 where TMPDIR is a gfs2 mountpoint.
 * Try it on other filesystems to compare results.
 */

#include <sys/wait.h>
#include <pthread.h>

#include "tst_safe_pthread.h"
#include "tst_test.h"

static int fd;

static void *do_thread1(void *arg)
{
	const struct flock fl = {
		.l_type = F_WRLCK,
		.l_whence = SEEK_SET,
		.l_start = 0L,
		.l_len = 1L,
	};
	(void)arg;

	tst_res(TINFO, "thread1 waits for thread2 to lock 0-0");
	TST_CHECKPOINT_WAIT(1);

	tst_res(TINFO, "thread1 lock region 0-0 - It should block");
	SAFE_FCNTL(fd, F_SETLKW, &fl);
	tst_res(TINFO, "lock region 0-0 acquired");

	return NULL;
}

static void *do_thread2(void *arg)
{
	const struct flock fl = {
		.l_type = F_WRLCK,
		.l_whence = SEEK_SET,
		.l_start = 0L,
		.l_len = 1L,
	};
	(void)arg;

	tst_res(TINFO, "thread1 lock region 0-0 - It should block");
	SAFE_FCNTL(fd, F_SETLKW, &fl);
	tst_res(TINFO, "lock region 0-0 acquired");

	return NULL;
}

void do_child(void)
{
	pthread_t t1, t2;

	SAFE_PTHREAD_CREATE(&t1, NULL, do_thread1, NULL);
	SAFE_PTHREAD_CREATE(&t2, NULL, do_thread2, NULL);

	SAFE_PTHREAD_JOIN(t1, NULL);
	SAFE_PTHREAD_JOIN(t2, NULL);

	tst_res(TPASS, "Child passed!");
}

void do_parent(void)
{
	struct flock fl = {
		.l_whence = SEEK_SET,
	};

	/* wait for 1 seconds so thread2 lock 0-0 tries to acquires at first
	 * than thread1 lock 0-0 tries to acquired to have a specific waiting
	 * order in dlm posix handling.
	 */
	sleep(1);
	/* tell thread2 to call SETLKW for lock 0-0 */
	TST_CHECKPOINT_WAKE(1);
	/* wait 3 seconds for thread 1 and 2 being in waiting state */
	sleep(3);

	/* unlock 0-0, should be successful */
	fl.l_type = F_UNLCK;
	fl.l_start = 0;
	fl.l_len = 1;
	tst_res(TINFO, "unlock region 0-0 thread2");
	SAFE_FCNTL(fd, F_SETLK, &fl);
}

static void fcntl40_test(void)
{
	struct flock fl = {
		.l_type = F_WRLCK,
		.l_whence = SEEK_SET,
		.l_start = 0L,
		.l_len = 2L,
	};
	pid_t pid;

	tst_res(TINFO, "parent lock region 0-1 - should be successful");
	SAFE_FCNTL(fd, F_SETLK, &fl);
	tst_res(TINFO, "parent region 0-1 locked");

	pid = SAFE_FORK();
	if (pid == 0) {
		do_child();
		return;
	}

	do_parent();
	wait(NULL);

	tst_res(TPASS, "Parent passed!");
}

static void setup(void)
{
	fd = SAFE_OPEN("filename", O_RDWR | O_CREAT, 0700);
}

static void cleanup(void)
{
	if (fd > -1)
		SAFE_CLOSE(fd);
}

static struct tst_test test = {
	.forks_child = 1,
	.needs_checkpoints = 1,
	.test_all = fcntl40_test,
	.setup = setup,
	.cleanup = cleanup,
};
