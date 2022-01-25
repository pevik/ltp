#define _GNU_SOURCE     /* Needed to get O_LARGEFILE definition */
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/fanotify.h>

int main(int argc, char *argv[])
{
	char buf;
	int fd;
	while (1) {

		/* Create the file descriptor for accessing the fanotify API */
		fd = fanotify_init(FAN_CLOEXEC | FAN_CLASS_CONTENT |
				FAN_NONBLOCK, O_RDONLY | O_LARGEFILE);
		if (fd == -1)
			perror("fanotify_init");

		if (fanotify_mark(fd, FAN_MARK_ADD | FAN_MARK_MOUNT,
				FAN_ACCESS | FAN_MODIFY | FAN_OPEN_PERM |
				FAN_CLOSE | FAN_OPEN | FAN_ACCESS_PERM |
				FAN_ONDIR | FAN_EVENT_ON_CHILD, -1,
				argv[1]) == -1)
			perror("fanotify_mark");

		close(fd);
	}

	exit(EXIT_SUCCESS);
}
