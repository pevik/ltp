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

	/* Loop marking all kinds of events and flush */
	while (1) {

		if (fanotify_mark(fd, FAN_MARK_ADD | FAN_MARK_MOUNT,
			  FAN_ACCESS | FAN_MODIFY | FAN_OPEN_PERM | FAN_CLOSE |
			  FAN_OPEN | FAN_ACCESS_PERM | FAN_ONDIR |
			  FAN_EVENT_ON_CHILD, -1, argv[1]) == -1)

			perror("fanotify_mark add");

		if (fanotify_mark(fd, FAN_MARK_FLUSH | FAN_MARK_MOUNT,
						0, -1, argv[1]) == -1)
			perror("fanotify_mark flush mount");

		if (fanotify_mark(fd, FAN_MARK_ADD | FAN_MARK_MOUNT,
			  FAN_ACCESS | FAN_MODIFY | FAN_OPEN_PERM | FAN_CLOSE |
			  FAN_OPEN | FAN_ACCESS_PERM | FAN_ONDIR |
			  FAN_EVENT_ON_CHILD, -1, argv[1]) == -1)

			perror("fanotify_mark add");

		if (fanotify_mark(fd, FAN_MARK_FLUSH, 0, -1, argv[1]) == -1)
			perror("fanotify_mark flush");
	}

	close(fd);
	exit(EXIT_SUCCESS);
}
