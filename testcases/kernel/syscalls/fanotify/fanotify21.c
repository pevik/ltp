// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2021 Google. All Rights Reserved.
 *
 * Started by Matthew Bobrowski <repnop@google.com>
 */

/*\
 * Test verifies whether the returned struct fanotify_event_info_pidfd in
 * :manpage:`fanotify(7)` FAN_REPORT_PIDFD mode contains the expected set of
 * information.
 *
 * NOTE: FAN_REPORT_PIDFD support was added in v5.15-rc1 in
 * af579beb666a ("fanotify: add pidfd support to the fanotify API").
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <pthread.h>
#include "tst_test.h"
#include "tst_safe_stdio.h"
#include "tst_safe_macros.h"
#include "tst_safe_pthread.h"
#include "lapi/pidfd.h"

#ifdef HAVE_SYS_FANOTIFY_H
#include "fanotify.h"

#define BUF_SZ		4096
#define MOUNT_PATH	"fs_mnt"
#define TEST_FILE	MOUNT_PATH "/testfile"

struct pidfd_fdinfo_t {
	int pos;
	int flags;
	int mnt_id;
	int pid;
	int ns_pid;
};

static struct test_case_t {
	char *name;
	int trigger_in_child;
	int want_pidfd_err;
	int remount_ro;
} test_cases[] = {
	{
		"return a valid pidfd for event created by self",
		0,
		0,
		0,
	},
	{
		"return a valid pidfd for event created by child",
		1,
		0,
		0,
	},
	{
		"return invalid pidfd for event created by terminated child",
		1,
		1,
		0,
	},
	{
		"fail to open rw fd for event created on read-only mount",
		0,
		0,
		1,
	},
};

static int fanotify_fd;
static char event_buf[BUF_SZ];
static struct pidfd_fdinfo_t expected_pidfd_fdinfo;

static int fd_error_unsupported;
static int thread_pidfd_unsupported;

#define TST_VARIANT_FD_ERROR (tst_variant & 1)
#define TST_VARIANT_PIDFD_THREAD (tst_variant & 2)

static void read_pidfd_fdinfo(int pidfd, struct pidfd_fdinfo_t *pidfd_fdinfo)
{
	char *fdinfo_path;

	SAFE_ASPRINTF(&fdinfo_path, "/proc/self/fdinfo/%d", pidfd);
	SAFE_FILE_LINES_SCANF(fdinfo_path, "pos: %d", &pidfd_fdinfo->pos);
	SAFE_FILE_LINES_SCANF(fdinfo_path, "flags: %d", &pidfd_fdinfo->flags);
	SAFE_FILE_LINES_SCANF(fdinfo_path, "mnt_id: %d", &pidfd_fdinfo->mnt_id);
	SAFE_FILE_LINES_SCANF(fdinfo_path, "Pid: %d", &pidfd_fdinfo->pid);
	SAFE_FILE_LINES_SCANF(fdinfo_path, "NSpid: %d", &pidfd_fdinfo->ns_pid);

	free(fdinfo_path);
}

static void generate_event(void)
{
	int fd;

	/* Generate a single FAN_OPEN event on the watched object. */
	fd = SAFE_OPEN(TEST_FILE, O_RDONLY);
	SAFE_CLOSE(fd);
}

static pid_t do_fork(int want_pidfd_err)
{
	int pidfd;
	pid_t child;

	child = SAFE_FORK();
	if (child == 0) {
		SAFE_CLOSE(fanotify_fd);
		generate_event();
		TST_CHECKPOINT_WAIT(0);
		exit(EXIT_SUCCESS);
	}

	pidfd = SAFE_PIDFD_OPEN(child, 0);
	read_pidfd_fdinfo(pidfd, &expected_pidfd_fdinfo);
	SAFE_CLOSE(pidfd);

	if (want_pidfd_err) {
		int status;
		TST_CHECKPOINT_WAKE(0);
		SAFE_WAITPID(child, &status, 0);
		if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
			tst_brk(TBROK, "child process terminated incorrectly");

		return -1;
	}

	return child;
}

static void *thread_generate_event(void *arg)
{
	*(int *)arg = SAFE_PIDFD_OPEN(gettid(), PIDFD_THREAD);
	TST_CHECKPOINT_WAKE(0);

	generate_event();
	TST_CHECKPOINT_WAIT(0);
	pthread_exit(0);
}

static pthread_t do_pthread_create(int want_pidfd_err)
{
	int pidfd;
	pthread_t worker;

	SAFE_PTHREAD_CREATE(&worker, NULL, thread_generate_event, &pidfd);

	TST_CHECKPOINT_WAIT(0);
	read_pidfd_fdinfo(pidfd, &expected_pidfd_fdinfo);

	if (want_pidfd_err) {
		int status;
		struct pidfd_fdinfo_t thread_pidfd_fdinfo;
		TST_CHECKPOINT_WAKE(0);
		SAFE_PTHREAD_JOIN(worker, (void **)&status);
		if (status != 0)
			tst_brk(TBROK, "worker thread terminated incorrectly");

		/*
		 * Unlike waitpid(), pthread_join() only waits until the worker thread
		 * has exited from the pthread point of view. The thread may still be
		 * visible through its pidfd for a short time afterwards, and fanotify
		 * creates the event pidfd when the event is read. Wait until the
		 * worker pidfd fdinfo reports Pid: -1 before reading the event so
		 * that fanotify reports ESRCH/FAN_NOPIDFD instead of a pidfd.
		 */
		do {
			read_pidfd_fdinfo(pidfd, &thread_pidfd_fdinfo);
		} while (thread_pidfd_fdinfo.pid != -1);

		SAFE_CLOSE(pidfd);

		return -1;
	}

	SAFE_CLOSE(pidfd);

	return worker;
}

static void do_setup(void)
{
	int init_flags = FAN_REPORT_PIDFD;

	if (TST_VARIANT_FD_ERROR) {
		fanotify_fd = -1;
		fd_error_unsupported = fanotify_init_flags_supported_on_fs(FAN_REPORT_FD_ERROR, ".");
		if (fd_error_unsupported)
			return;
		init_flags |= FAN_REPORT_FD_ERROR;
	}

	if (TST_VARIANT_PIDFD_THREAD) {
		fanotify_fd = -1;
		thread_pidfd_unsupported = fanotify_init_flags_supported_on_fs(
			FAN_REPORT_PIDFD | FAN_REPORT_TID, ".");
		if (thread_pidfd_unsupported)
			return;
		init_flags |= FAN_REPORT_TID;
	}

	SAFE_TOUCH(TEST_FILE, 0666, NULL);

	/*
	 * An explicit check for FAN_REPORT_PIDFD is performed early
	 * on in the test initialization as it's a prerequisite for
	 * all test cases.
	 */
	REQUIRE_FANOTIFY_INIT_FLAGS_SUPPORTED_ON_FS(FAN_REPORT_PIDFD,
						    TEST_FILE);

	fanotify_fd = SAFE_FANOTIFY_INIT(init_flags, O_RDWR);
	SAFE_FANOTIFY_MARK(fanotify_fd, FAN_MARK_ADD, FAN_OPEN, AT_FDCWD,
			   TEST_FILE);
}

static void do_test(unsigned int num)
{
	int i = 0, len;
	struct test_case_t *tc = &test_cases[num];
	int nopidfd_err = tc->want_pidfd_err ?
			  (TST_VARIANT_FD_ERROR ? -ESRCH : FAN_NOPIDFD) : 0;
	int fd_err = (tc->remount_ro && TST_VARIANT_FD_ERROR) ? -EROFS : 0;
	union {
		pid_t pid;
		pthread_t pthread_id;
	} worker_id;

	tst_res(TINFO, "Test #%d.%d: %s %s", num, tst_variant, tc->name,
		TST_VARIANT_FD_ERROR ? (TST_VARIANT_PIDFD_THREAD ?
			"(FAN_REPORT_FD_ERROR, FAN_REPORT_TID)" : "(FAN_REPORT_FD_ERROR)") :
			(TST_VARIANT_PIDFD_THREAD ? "(FAN_REPORT_TID)" : ""));

	if (fd_error_unsupported && TST_VARIANT_FD_ERROR) {
		FANOTIFY_INIT_FLAGS_ERR_MSG(FAN_REPORT_FD_ERROR, fd_error_unsupported);
		return;
	}

	if (thread_pidfd_unsupported && TST_VARIANT_PIDFD_THREAD) {
		FANOTIFY_INIT_FLAGS_ERR_MSG(FAN_REPORT_PIDFD | FAN_REPORT_TID,
				thread_pidfd_unsupported);
		return;
	}

	if (tc->remount_ro) {
		/* SAFE_MOUNT fails to remount FUSE */
		if (mount(tst_device->dev, MOUNT_PATH, tst_device->fs_type,
			  MS_REMOUNT|MS_RDONLY, NULL) != 0) {
			tst_brk(TFAIL,
				"filesystem %s failed to remount readonly",
				tst_device->fs_type);
		}
	}

	/*
	 * Generate the event either in the current task or in another task.
	 * When trigger_in_child is set, the event can be generated by either
	 * a child process or a worker thread depending on the test variant.
	 * The want_pidfd_err field determines whether the event-generating
	 * task is still valid when the event is read.
	 */
	if (tc->trigger_in_child) {
		if (TST_VARIANT_PIDFD_THREAD)
			worker_id.pthread_id = do_pthread_create(tc->want_pidfd_err);
		else
			worker_id.pid = do_fork(tc->want_pidfd_err);
	} else {
		/*
		 * Although the expected pid and the pid reported by fanotify are
		 * the same in this case, pidfds created with and without PIDFD_THREAD
		 * flag have different fdinfo flags. Use PIDFD_THREAD for the expected
		 * pidfd fdinfo so that the fdinfo can be compared bitwise.
		 */
		int pidfd = SAFE_PIDFD_OPEN(gettid(), TST_VARIANT_PIDFD_THREAD ? PIDFD_THREAD : 0);
		read_pidfd_fdinfo(pidfd, &expected_pidfd_fdinfo);
		SAFE_CLOSE(pidfd);

		generate_event();
	}

	/*
	 * Read all of the queued events into the provided event
	 * buffer.
	 */
	len = read(fanotify_fd, event_buf, sizeof(event_buf));
	if (len < 0) {
		if (tc->remount_ro && !fd_err && errno == EROFS) {
			tst_res(TPASS, "cannot read event with rw fd from a ro fs");
			return;
		}
		tst_brk(TBROK | TERRNO, "reading fanotify events failed");
	} else if (tc->remount_ro && !fd_err) {
		tst_res(TFAIL, "got unexpected event with rw fd from a ro fs");
	}
	while (i < len) {
		struct fanotify_event_metadata *event;
		struct fanotify_event_info_pidfd *info;
		struct pidfd_fdinfo_t event_pidfd_fdinfo;

		event = (struct fanotify_event_metadata *)&event_buf[i];
		info = (struct fanotify_event_info_pidfd *)(event + 1);

		/*
		 * Checks ensuring that pidfd information record object header
		 * fields are set correctly.
		 */
		if (info->hdr.info_type != FAN_EVENT_INFO_TYPE_PIDFD) {
			tst_res(TFAIL,
				"unexpected info_type received in info "
				"header (expected: %d, got: %d",
				FAN_EVENT_INFO_TYPE_PIDFD,
				info->hdr.info_type);
			info = NULL;
			goto next_event;
		} else if (info->hdr.len !=
			   sizeof(struct fanotify_event_info_pidfd)) {
			tst_res(TFAIL,
				"unexpected info object length "
				"(expected: %lu, got: %d",
				sizeof(struct fanotify_event_info_pidfd),
				info->hdr.len);
			info = NULL;
			goto next_event;
		}

		/*
		 * Check if event->fd reported any errors during
		 * creation and whether they're expected.
		 */
		if (!fd_err && event->fd >= 0) {
			tst_res(TPASS,
				"event->fd %d is valid as expected",
				event->fd);
		} else if (fd_err && event->fd == fd_err) {
			tst_res(TPASS,
				"event->fd is error %d as expected",
				event->fd);
		} else if (fd_err) {
			tst_res(TFAIL,
				"event->fd is %d, but expected error %d",
				event->fd, fd_err);
		} else {
			tst_res(TFAIL,
				"event->fd creation failed with error %d",
				event->fd);
		}

		/*
		 * Check if pidfd information object reported any errors during
		 * creation and whether they're expected.
		 */
		if (info->pidfd < 0 && !tc->want_pidfd_err) {
			tst_res(TFAIL,
				"pidfd creation failed for pid: %u with pidfd error value "
				"set to: %d",
				(unsigned int)event->pid,
				info->pidfd);
			goto next_event;
		} else if (tc->want_pidfd_err && info->pidfd != nopidfd_err) {
			tst_res(TFAIL,
				"pidfd set to an unexpected error: %d for pid: %u",
				info->pidfd,
				(unsigned int)event->pid);
			goto next_event;
		} else if (tc->want_pidfd_err && info->pidfd == nopidfd_err) {
			tst_res(TPASS,
				"pid: %u terminated before pidfd was created, "
				"pidfd set to the value of: %d, as expected",
				(unsigned int)event->pid,
				nopidfd_err);
			goto next_event;
		}

		/*
		 * No pidfd errors occurred, continue with verifying pidfd
		 * fdinfo validity.
		 */
		read_pidfd_fdinfo(info->pidfd, &event_pidfd_fdinfo);
		if (event_pidfd_fdinfo.pid != event->pid) {
			tst_res(TFAIL,
				"pidfd provided for incorrect pid "
				"(expected pidfd for pid: %u, got pidfd for "
				"pid: %u)",
				(unsigned int)event->pid,
				(unsigned int)event_pidfd_fdinfo.pid);
			goto next_event;
		} else if (memcmp(&event_pidfd_fdinfo, &expected_pidfd_fdinfo,
				  sizeof(struct pidfd_fdinfo_t))) {
			tst_res(TFAIL,
				"pidfd fdinfo values for self and event differ "
				"(expected pos: %d, flags: %x, mnt_id: %d, "
				"pid: %d, ns_pid: %d, got pos: %d, "
				"flags: %x, mnt_id: %d, pid: %d, ns_pid: %d",
				expected_pidfd_fdinfo.pos,
				expected_pidfd_fdinfo.flags,
				expected_pidfd_fdinfo.mnt_id,
				expected_pidfd_fdinfo.pid,
				expected_pidfd_fdinfo.ns_pid,
				event_pidfd_fdinfo.pos,
				event_pidfd_fdinfo.flags,
				event_pidfd_fdinfo.mnt_id,
				event_pidfd_fdinfo.pid,
				event_pidfd_fdinfo.ns_pid);
			goto next_event;
		} else {
			tst_res(TPASS,
				"got an event with a valid pidfd info record, "
				"mask: %lld, pid: %u, fd: %d, "
				"pidfd: %d, info_type: %d, info_len: %d",
				(unsigned long long)event->mask,
				(unsigned int)event->pid,
				event->fd,
				info->pidfd,
				info->hdr.info_type,
				info->hdr.len);
		}

next_event:
		i += event->event_len;
		if (event->fd >= 0)
			SAFE_CLOSE(event->fd);

		if (info && info->pidfd >= 0)
			SAFE_CLOSE(info->pidfd);
	}

	if (tc->trigger_in_child && !tc->want_pidfd_err) {
		int status;
		TST_CHECKPOINT_WAKE(0);
		if (TST_VARIANT_PIDFD_THREAD) {
			SAFE_PTHREAD_JOIN(worker_id.pthread_id, (void **)&status);
			if (status != 0)
				tst_brk(TBROK, "worker thread terminated incorrectly");
		} else {
			SAFE_WAITPID(worker_id.pid, &status, 0);
			if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
				tst_brk(TBROK, "child process terminated incorrectly");
		}
	}
}

static void do_cleanup(void)
{
	if (fanotify_fd >= 0)
		SAFE_CLOSE(fanotify_fd);
}

static struct tst_test test = {
	.setup = do_setup,
	.test = do_test,
	.tcnt = ARRAY_SIZE(test_cases),
	.test_variants = 4,
	.cleanup = do_cleanup,
	.all_filesystems = 1,
	.needs_root = 1,
	.needs_checkpoints = 1,
	.mount_device = 1,
	.mntpoint = MOUNT_PATH,
	.forks_child = 1,
};

#else
	TST_TEST_TCONF("system doesn't have required fanotify support");
#endif /* HAVE_SYS_FANOTIFY_H */
