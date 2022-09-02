// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2022 Red Hat, Inc.  All Rights Reserved.
 * Author: Murphy Zhou <jencce.kernel@gmail.com>
 * Copyright (c) Linux Test Project, 2001-2022
 */

/*\
 * [Description]
 *
 * This is an irregular stress test for Linux kernel fanotify/inotify
 * interfaces. It calls thoese interfaces with possible best coverage
 * arguments, in a loop. It ignores some return values in the loop to
 * let the stress going on. At the same time, it initiates IO traffics
 * by calling IO syscalls.
 *
 * If kernel does no panic or hang after the test, test pass.
 *
 * It detected a leak in fsnotify code which was fixed by Amir through
 * this Linux commit:
 *     4396a731 fsnotify: fix sb_connectors leak
 *
 */

#define _GNU_SOURCE     /* Needed to get O_LARGEFILE definition */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <sys/fanotify.h>
#include <sys/inotify.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>

#include "tst_test.h"
#include "../../syscalls/fanotify/fanotify.h"
#include "../../syscalls/inotify/inotify.h"

static int fd0;

#define TESTDIR "testdir"
#define TESTFILE "testdir/file"

static void cleanup(void)
{
	if (fd0 > 0) {
		SAFE_CLOSE(fd0);
	}
}

static void setup(void)
{
	SAFE_MKDIR(TESTDIR, 0777);
	fd0 = SAFE_OPEN(TESTFILE, O_CREAT|O_RDWR, 0666);
}

static void fanotify_flushes(char *fn)
{
	int fd;

	fd = SAFE_FANOTIFY_INIT(FAN_CLOEXEC | FAN_CLASS_CONTENT | FAN_NONBLOCK,
					   O_RDONLY | O_LARGEFILE);

	while (tst_remaining_runtime() > 10) {
		/* As a stress test, we ignore the return values here to
		 * proceed with the stress.
		 */
		fanotify_mark(fd, FAN_MARK_ADD | FAN_MARK_MOUNT,
			  FAN_ACCESS | FAN_MODIFY | FAN_OPEN_PERM | FAN_CLOSE |
			  FAN_OPEN | FAN_ACCESS_PERM | FAN_ONDIR |
			  FAN_EVENT_ON_CHILD, -1, fn);

		fanotify_mark(fd, FAN_MARK_FLUSH | FAN_MARK_MOUNT,
						0, -1, fn);

		fanotify_mark(fd, FAN_MARK_ADD | FAN_MARK_MOUNT,
			  FAN_ACCESS | FAN_MODIFY | FAN_OPEN_PERM | FAN_CLOSE |
			  FAN_OPEN | FAN_ACCESS_PERM | FAN_ONDIR |
			  FAN_EVENT_ON_CHILD, -1, fn);

		fanotify_mark(fd, FAN_MARK_FLUSH, 0, -1, fn);
	}

	close(fd);
	exit(EXIT_SUCCESS);
}

static void fanotify_inits(char *fn)
{
	int fd;

	while (tst_remaining_runtime() > 10) {
		/* As a stress test, we ignore the return values here to
		 * proceed with the stress.
		 */
		fd = fanotify_init(FAN_CLOEXEC | FAN_CLASS_CONTENT |
				FAN_NONBLOCK, O_RDONLY | O_LARGEFILE);
		fanotify_mark(fd, FAN_MARK_ADD | FAN_MARK_MOUNT,
				FAN_ACCESS | FAN_MODIFY | FAN_OPEN_PERM |
				FAN_CLOSE | FAN_OPEN | FAN_ACCESS_PERM |
				FAN_ONDIR | FAN_EVENT_ON_CHILD, -1, fn);
		close(fd);
	}
	exit(EXIT_SUCCESS);
}

static void add_mark(int fd, uint64_t mask, char *path)
{
	fanotify_mark(fd, FAN_MARK_ADD, mask, -1, path);
}

static void remove_mark(int fd, uint64_t mask, char *path)
{
	fanotify_mark(fd, FAN_MARK_REMOVE, mask, -1, path);
}

static void fanotify_marks(char *fn)
{
	int fd;

	fd = SAFE_FANOTIFY_INIT(FAN_CLOEXEC | FAN_CLASS_CONTENT | FAN_NONBLOCK,
					   O_RDONLY | O_LARGEFILE);
	while (tst_remaining_runtime() > 10) {
		add_mark(fd, FAN_ACCESS, fn);
		remove_mark(fd, FAN_ACCESS, fn);
		add_mark(fd, FAN_MODIFY, fn);
		remove_mark(fd, FAN_MODIFY, fn);
		add_mark(fd, FAN_OPEN_PERM, fn);
		remove_mark(fd, FAN_OPEN_PERM, fn);
		add_mark(fd, FAN_CLOSE, fn);
		remove_mark(fd, FAN_CLOSE, fn);
		add_mark(fd, FAN_OPEN, fn);
		remove_mark(fd, FAN_OPEN, fn);
		add_mark(fd, FAN_ACCESS_PERM, fn);
		remove_mark(fd, FAN_ACCESS_PERM, fn);
		add_mark(fd, FAN_ONDIR, fn);
		remove_mark(fd, FAN_ONDIR, fn);
		add_mark(fd, FAN_EVENT_ON_CHILD, fn);
		remove_mark(fd, FAN_EVENT_ON_CHILD, fn);
	}
	close(fd);
	exit(EXIT_SUCCESS);
}

/* Read all available fanotify events from the file descriptor 'fd' */
static void fa_handle_events(int fd)
{
	const struct fanotify_event_metadata *metadata;
	struct fanotify_event_metadata buf[200];
	ssize_t len;
	struct fanotify_response response;

	/* Loop while events can be read from fanotify file descriptor */
	for (;;) {
		/* Read some events */
		len = read(fd, (void *) &buf, sizeof(buf));
		if (len == -1 && errno != EAGAIN)
			tst_brk(TBROK | TERRNO, "fanotify read events failed");
		/* Check if end of available data reached */
		if (len <= 0)
			break;
		/* Point to the first event in the buffer */
		metadata = buf;
		/* Loop over all events in the buffer */
		while (FAN_EVENT_OK(metadata, len)) {
			if (metadata->vers != FANOTIFY_METADATA_VERSION) {
				tst_brk(TBROK | TERRNO,
				"Mismatch of fanotify metadata version.\n");
			}
			/* metadata->fd contains either FAN_NOFD, indicating a
			 * queue overflow, or a file descriptor (a nonnegative
			 * integer). Here, we simply ignore queue overflow.
			 */
			if (metadata->fd >= 0) {
				/* Handle open permission event */
				if (metadata->mask & FAN_OPEN_PERM) {
					/* Allow file to be opened */
					response.fd = metadata->fd;
					response.response = FAN_ALLOW;
					write(fd, &response,
					    sizeof(struct fanotify_response));
				}

				/* Handle access permission event */
				if (metadata->mask & FAN_ACCESS_PERM) {
					/* Allow file to be accessed */
					response.fd = metadata->fd;
					response.response = FAN_ALLOW;
					write(fd, &response,
					    sizeof(struct fanotify_response));
				}
				/* Ignore read/write access events */
				/* Close the file descriptor of the event */
				close(metadata->fd);
			}
			/* Advance to next event */
			metadata = FAN_EVENT_NEXT(metadata, len);
		}
	}
}

/* This is from fanotify(7) man page example */
static void fanotify_watch(void)
{
	int fd, poll_num, ecnt = 0;
	nfds_t nfds;
	struct pollfd fds[2];

	fd = SAFE_FANOTIFY_INIT(FAN_CLOEXEC | FAN_CLASS_CONTENT | FAN_NONBLOCK,
					   O_RDONLY | O_LARGEFILE);
	/* Mark the mount for:
	 * - permission events before opening files
	 * - notification events after closing a write-enabled file descriptor
	 */
	SAFE_FANOTIFY_MARK(fd, FAN_MARK_ADD | FAN_MARK_MOUNT,
			FAN_ACCESS | FAN_MODIFY | FAN_OPEN_PERM |
			FAN_CLOSE | FAN_OPEN | FAN_ACCESS_PERM |
			FAN_ONDIR | FAN_EVENT_ON_CHILD, -1, "/");

	nfds = 1;
	fds[0].fd = fd;
	fds[0].events = POLLIN;

	while (tst_remaining_runtime() > 10) {
		poll_num = poll(fds, nfds, 10);
		if (poll_num == -1)
			tst_brk(TBROK | TERRNO, "fanotify watch poll failed");
		if (poll_num > 0) {
			if (fds[0].revents & POLLIN) {
				/* Fanotify events are available */
				fa_handle_events(fd);
				ecnt++;
			}
		}
	}
	tst_printf("Got %d fanotify events\n", ecnt);
	tst_flush();
	exit(EXIT_SUCCESS);
}

static void freadfiles(char *fn)
{
	char buf[BUFSIZ];
	FILE *f;

	memset(buf, 1, BUFSIZ);
	while (tst_remaining_runtime() > 10) {
		f = fopen(fn, "r+");
		if (f == NULL)
			continue;
		fread(buf, sizeof(char), BUFSIZ, f);
		usleep(1);
		fclose(f);
	}
}

static void fwritefiles(char *fn)
{
	char buf[BUFSIZ];
	FILE *f;

	memset(buf, 1, BUFSIZ);
	while (tst_remaining_runtime() > 10) {
		f = fopen(fn, "w+");
		if (f == NULL)
			continue;
		fwrite(buf, sizeof(char), BUFSIZ, f);
		usleep(1);
		fclose(f);
	}
}

static void readfiles(char *fn)
{
	int fd;
	char buf[BUFSIZ];

	memset(buf, 1, BUFSIZ);
	while (tst_remaining_runtime() > 10) {
		fd = open(fn, O_RDONLY);
		if (fd == -1)
			continue;
		read(fd, buf, BUFSIZ);
		usleep(1);
		close(fd);
	}
}

static void writefiles(char *fn)
{
	int fd;
	char buf[BUFSIZ];

	memset(buf, 1, BUFSIZ);
	while (tst_remaining_runtime() > 10) {
		fd = open(fn, O_RDWR);
		if (fd == -1)
			continue;
		write(fd, buf, BUFSIZ);
		usleep(1);
		close(fd);
	}
}

static void inotify_add_rm(char *fn)
{
	int notify_fd;
	int wd;

	notify_fd = SAFE_MYINOTIFY_INIT1(IN_CLOEXEC);

	while (tst_remaining_runtime() > 10) {
		wd = inotify_add_watch(notify_fd, fn,
			IN_ACCESS | IN_ATTRIB | IN_CLOSE_WRITE |
			IN_CLOSE_NOWRITE | IN_CREATE | IN_DELETE |
			IN_DELETE_SELF | IN_MODIFY | IN_MOVE_SELF |
			IN_MOVED_FROM | IN_MOVED_TO | IN_OPEN);

		inotify_rm_watch(notify_fd, wd);
	}
	close(notify_fd);
}

static void inotify_inits(void)
{
	int notify_fd;

	while (tst_remaining_runtime() > 10) {
		notify_fd = inotify_init1(IN_CLOEXEC);
		close(notify_fd);
	}
}

static void inotify_add_rm_watches(char *fn)
{
	int fd, wd;

	fd = SAFE_MYINOTIFY_INIT();

	while (tst_remaining_runtime() > 10) {
		wd = inotify_add_watch(fd, fn, IN_MODIFY);
		inotify_rm_watch(fd, wd);
	}
	close(fd);
}

static void i_handle_events(int fd)
{
	char buf[4096]
		__attribute__((aligned(__alignof__(struct inotify_event))));
	ssize_t len;

	/* Loop while events can be read from inotify file descriptor. */
	for (;;) {
		len = read(fd, buf, sizeof(buf));
		if (len == -1 && errno != EAGAIN)
			tst_brk(TBROK | TERRNO, "inotify read event failed");
		/* If the nonblocking read() found no events to read, then
		 * it returns -1 with errno set to EAGAIN. In that case,
		 * we exit the loop.
		 */
		if (len <= 0)
			break;
	}
}

static void inotify_watch(char *fn)
{
	int fd, poll_num, wd, ecnt = 0;
	nfds_t nfds;
	struct pollfd fds[2];

	fd = SAFE_MYINOTIFY_INIT1(IN_NONBLOCK);

	/* Mark directories for events
	 * - file was opened
	 * - file was closed
	 */
	wd = SAFE_MYINOTIFY_ADD_WATCH(fd, fn, IN_OPEN | IN_CLOSE);

	nfds = 2;
	fds[0].fd = STDIN_FILENO;       /* Console input */
	fds[0].events = POLLIN;
	fds[1].fd = fd;                 /* Inotify input */
	fds[1].events = POLLIN;

	/* Wait for events and/or terminal input. */
	while (tst_remaining_runtime() > 10) {
		poll_num = poll(fds, nfds, 10);
		if (poll_num == -1)
			tst_brk(TBROK | TERRNO, "inotify watch poll failed");
		if (poll_num > 0) {
			if (fds[1].revents & POLLIN) {
				/* Inotify events are available. */
				i_handle_events(fd);
				ecnt++;
			}
		}
	}

	inotify_rm_watch(fd, wd);
	close(fd);
	tst_printf("Got %d inotify events\n", ecnt);
	tst_flush();
	exit(EXIT_SUCCESS);
}

struct tcase {
	char *desc;
	void (*func_test)(char *fn);
	int ondir;  /* run stress on directory */
	int onfile;  /* run stress on file */
};
static struct tcase tcases[] = {
	{"fanotify_flush stress", fanotify_flushes, 1, 1},
	{"fanotify_init stress", fanotify_inits, 1, 1},
	{"fanotify_mark stress", fanotify_marks, 1, 1},
	{"fanotify watching stress", fanotify_watch, 1, 0},
	{"fread stress", freadfiles, 0, 1},
	{"fwrite stress", fwritefiles, 0, 1},
	{"inotify add rm stress", inotify_add_rm, 1, 1},
	{"inotify init stress", inotify_inits, 1, 1},
	{"inotify add rm watch stress", inotify_add_rm_watches, 1, 1},
	{"inotify watching stress", inotify_watch, 1, 0},
	{"read stress", readfiles, 0, 1},
	{"write stress", writefiles, 0, 1}
};

static void run(void)
{
	int tcnt = ARRAY_SIZE(tcases);
	int i = 0;
	const struct tst_clone_args args = {
		.exit_signal = SIGCHLD,
	};

	tst_res(TINFO, "Starting %d stresses\n", tcnt);

	while (i < tcnt) {
		if (tcases[i].ondir && !SAFE_CLONE(&args)) {
			tst_res(TINFO, "Starting %s on dir\n", tcases[i].desc);
			tcases[i].func_test(TESTDIR);
			tst_res(TINFO, "Ending %s on dir\n", tcases[i].desc);
			tst_flush();
			exit(EXIT_SUCCESS);
		}
		if (tcases[i].onfile && !SAFE_CLONE(&args)) {
			tst_res(TINFO, "Starting %s on file\n", tcases[i].desc);
			tcases[i].func_test(TESTFILE);
			tst_res(TINFO, "Ending %s on file\n", tcases[i].desc);
			exit(EXIT_SUCCESS);
		}
		i++;
	}
	tst_reap_children();
	tst_res(TPASS, "No panic no hang, test PASS");
}

static struct tst_test test = {
	.tcnt = 1,
	.max_runtime = 60,
	.forks_child = 1,
	.needs_root = 1,
	.needs_tmpdir = 1,
	.test = run,
	.setup = setup,
	.cleanup = cleanup,
	.tags = (const struct tst_tag[]) {
		{"linux-git", "4396a731 "},
		{},
	},
};
