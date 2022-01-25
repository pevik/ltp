#define _GNU_SOURCE     /* Needed to get O_LARGEFILE definition */
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/fanotify.h>
#include <unistd.h>
#include <string.h>

void add_mark(int fd, uint64_t mask, char *path)
{
	if (fanotify_mark(fd, FAN_MARK_ADD, mask, -1, path) == -1)
		perror("fanotify_mark add");
}

void remove_mark(int fd, uint64_t mask, char *path)
{
	if (fanotify_mark(fd, FAN_MARK_REMOVE, mask, -1, path) == -1)
		perror("fanotify_mark remove");
}

int main(int argc, char *argv[])
{
	char buf;
	int fd;
	/* Create the file descriptor for accessing the fanotify API */
	fd = fanotify_init(FAN_CLOEXEC | FAN_CLASS_CONTENT | FAN_NONBLOCK,
					   O_RDONLY | O_LARGEFILE);
	if (fd == -1) {
		perror("fanotify_init");
		exit(EXIT_FAILURE);
	}

	/* Loop marking all kinds of events */
	while (1) {
		add_mark(fd, FAN_ACCESS, argv[1]);
		remove_mark(fd, FAN_ACCESS, argv[1]);
		add_mark(fd, FAN_MODIFY, argv[1]);
		remove_mark(fd, FAN_MODIFY, argv[1]);
		add_mark(fd, FAN_OPEN_PERM, argv[1]);
		remove_mark(fd, FAN_OPEN_PERM, argv[1]);
		add_mark(fd, FAN_CLOSE, argv[1]);
		remove_mark(fd, FAN_CLOSE, argv[1]);
		add_mark(fd, FAN_OPEN, argv[1]);
		remove_mark(fd, FAN_OPEN, argv[1]);
		add_mark(fd, FAN_ACCESS_PERM, argv[1]);
		remove_mark(fd, FAN_ACCESS_PERM, argv[1]);
		add_mark(fd, FAN_ONDIR, argv[1]);
		remove_mark(fd, FAN_ONDIR, argv[1]);
		add_mark(fd, FAN_EVENT_ON_CHILD, argv[1]);
		remove_mark(fd, FAN_EVENT_ON_CHILD, argv[1]);
	}

	close(fd);
	exit(EXIT_SUCCESS);
}
