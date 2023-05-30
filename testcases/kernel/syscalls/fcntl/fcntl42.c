/*
 * [Description]
 * This test confirms that DLM posix locking has problems when a posix lock
 * got interrupted every lock gets unlocked.
 *
 * man fcntl:
 *
 * F_SETLKW (struct flock *)
 *  As for F_SETLK, but if a conflicting lock is held on  the  file,
 *  then  wait  for that lock to be released.  If a signal is caught
 *  while waiting, then the call is interrupted and (after the  signal
 *  handler has returned) returns immediately (with return value -1 and
 *  errno set to EINTR; see signal(7)).
 *
 * The above quote of the man page of fcntl() is what this testcase tests.
 * particulary if it has side-effects of previously acquired locks.
 *
 * [How to use it]
 * Call it with TMPDIR=/mnt ./fcntl42 where TMPDIR is a gfs2 mountpoint.
 * Try it on other filesystems to compare results.
 *
 * [What's it doing]
 *
 * What this test does is (using dlm_controld interval range interpretation):
 *
 * parent:
 *
 * 1. lock[0-0]
 *
 * child:
 *
 * 2. lock[1-1]
 * 3. lockw[0-0] - should block (see 1. parent), but we get interrupted by SIGALRM
 *
 * parent:
 *
 * 4. trylock[1-1] - should return -1 and errno -EAGAIN because the child
 *                   should still have lock[1-1] acuired and this is what
 *                   the child thinks to have. If it's successful the child
 *                   wrongly assumes it has the lock[1-1] still acquired and
 *                   the child process is still alive.
 *
 */

#include <sys/wait.h>

#include "tst_test.h"

static int fd;

static void catch_alarm(int num)
{
	(void)num;

	tst_res(TINFO, "catch alarm");
}

void do_child1(void)
{
	struct flock fl = {
		.l_type = F_WRLCK,
		.l_whence = SEEK_SET,
		.l_start = 1L,
		.l_len = 1L,
	};
	struct sigaction act;
	int rv;

	SAFE_FCNTL(fd, F_SETLK, &fl);
	tst_res(TINFO, "child locked lock 1-1");

	fl.l_type = F_WRLCK;
	fl.l_start = 0;
	fl.l_len = 1;

	memset(&act, 0, sizeof(act));
	act.sa_handler = catch_alarm;
	sigemptyset(&act.sa_mask);
	sigaddset(&act.sa_mask, SIGALRM);
	sigaction(SIGALRM, &act, NULL);

	/* interrupt SETLKW by signal in 3 secs */
	alarm(3);
	rv = fcntl(fd, F_SETLKW, &fl);
	if (rv == -1 && errno == EINTR)
		tst_res(TPASS, "Child1 interrupted 0-0");

	TST_CHECKPOINT_WAKE(1);
	/* keep child alive until parent check region */
	TST_CHECKPOINT_WAIT(2);
}

static void fcntl40_test(void)
{
	struct flock fl = {
		.l_type = F_WRLCK,
		.l_whence = SEEK_SET,
		.l_start = 0L,
		.l_len = 1L,
	};
	pid_t pid;
	int rv;

	SAFE_FCNTL(fd, F_SETLK, &fl);
	tst_res(TINFO, "parent lock 0-0");

	pid = SAFE_FORK();
	if (pid == 0) {
		do_child1();
		tst_res(TINFO, "childs exits");
		return;
	}

	TST_CHECKPOINT_WAIT(1);

	fl.l_type = F_WRLCK;
	fl.l_start = 1;
	fl.l_len = 1;
	rv = fcntl(fd, F_SETLK, &fl);
	/* parent testing childs region, the child will think
	 * it has region 1-1 locked because it was interrupted
	 * by region 0-0. Due bugs the interruption also unlocked
	 * region 1-1.
	 */
	if (rv == -1 && errno == EAGAIN)
		tst_res(TPASS, "region 1-1 locked");
	else
		tst_res(TFAIL, "region 1-1 unlocked");

	TST_CHECKPOINT_WAKE(2);

	wait(NULL);
	tst_res(TINFO, "parent exits");
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
