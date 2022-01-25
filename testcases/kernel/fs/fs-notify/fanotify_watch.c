/* Example in man 7 fanotify */

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

/* get comm for pid from /proc/pid/comm */
static char * get_comm_pid(unsigned int pid)
{
	char * proc_name;
	char * comm;
	int comm_fd, cnt;

	proc_name = (char *)malloc(50);
	if (proc_name != NULL)
		sprintf(proc_name, "/proc/%u/comm\0", pid);
	else
		return NULL;

	comm = (char *)malloc(50);
	if (comm != NULL)
		memset(comm, 0, 50);
	else
		return NULL;

	comm_fd = open(proc_name, O_RDONLY);
	if (comm_fd != -1) {
		cnt = read(comm_fd, comm, 50);
		if (cnt != -1) {
			comm[cnt] = '\0';
			close(comm_fd);
			return comm;
		}
		close(comm_fd);
	}

	return NULL;
}

/* Read all available fanotify events from the file descriptor 'fd' */

static void handle_events(int fd)
{
	const struct fanotify_event_metadata *metadata;
	struct fanotify_event_metadata buf[200];
	ssize_t len;
	char path[PATH_MAX];
	ssize_t path_len;
	char procfd_path[PATH_MAX];
	struct fanotify_response response;

	/* Loop while events can be read from fanotify file descriptor */
	for(;;) {

		/* Read some events */
		len = read(fd, (void *) &buf, sizeof(buf));
		if (len == -1 && errno != EAGAIN) {
			perror("read");
			exit(EXIT_FAILURE);
		}

		/* Check if end of available data reached */
		if (len <= 0)
			break;

		/* Point to the first event in the buffer */
		metadata = buf;

		/* Loop over all events in the buffer */
		while (FAN_EVENT_OK(metadata, len)) {

			/* Check that run-time and compile-time structures match */

			if (metadata->vers != FANOTIFY_METADATA_VERSION) {
				fprintf(stderr,
				    "Mismatch of fanotify metadata version.\n");
				exit(EXIT_FAILURE);
			}

			/* metadata->fd contains either FAN_NOFD, indicating a
			   queue overflow, or a file descriptor (a nonnegative
			   integer). Here, we simply ignore queue overflow. */

			if (metadata->fd >= 0) {

				/* Handle open permission event */
				if (metadata->mask & FAN_OPEN_PERM) {
					printf("FAN_OPEN_PERM: ");

					/* Allow file to be opened */
					response.fd = metadata->fd;
					response.response = FAN_ALLOW;
					write(fd, &response,
					    sizeof(struct fanotify_response));
				}

				/* Handle access permission event */
				if (metadata->mask & FAN_ACCESS_PERM) {
					printf("FAN_ACCESS_PERM: ");

					/* Allow file to be accessed */
					response.fd = metadata->fd;
					response.response = FAN_ALLOW;
					write(fd, &response,
					    sizeof(struct fanotify_response));
				}

				/* Handle closing of writable file event */
				if (metadata->mask & FAN_CLOSE_WRITE)
					printf("FAN_CLOSE_WRITE: ");

				/* Handle closing of not writable file event */
				if (metadata->mask & FAN_CLOSE_NOWRITE)
					printf("FAN_CLOSE_NOWRITE: ");

				/* Handle modify file event */
				if (metadata->mask & FAN_MODIFY)
					printf("FAN_MODIFY: ");

				/* Handle open event */
				if (metadata->mask & FAN_OPEN)
					printf("FAN_OPEN: ");

				/* Handle access event */
				if (metadata->mask & FAN_ACCESS)
					printf("FAN_ACCESS: ");

				/* Handle access event */
				if (metadata->mask & FAN_ONDIR)
					printf("FAN_ONDIR: ");

				/* Handle access event */
				if (metadata->mask & FAN_EVENT_ON_CHILD)
					printf("FAN_EVENT_ON_CHILD: ");

				/* Retrieve/print the accessed file path*/
				snprintf(procfd_path, sizeof(procfd_path),
					    "/proc/self/fd/%d", metadata->fd);
				path_len = readlink(procfd_path, path,
					    sizeof(path) - 1);
				if (path_len == -1) {
					perror("readlink");
					exit(EXIT_FAILURE);
				}

				path[path_len] = '\0';
				printf("File %s.\t\t Comm %s", path,
					get_comm_pid(metadata->pid));

				/* Close the file descriptor of the event */

				close(metadata->fd);
			}

			/* Advance to next event */
			metadata = FAN_EVENT_NEXT(metadata, len);
		}
	}
}

int main(int argc, char *argv[])
{
	char buf;
	int fd, poll_num;
	nfds_t nfds;
	struct pollfd fds[2];
	FILE *f;
#if 0
	/* Check mount point is supplied */
	if (argc != 2) {
		fprintf(stderr, "Usage: %s MOUNT\n", argv[0]);
		exit(EXIT_FAILURE);
	}
#endif
	printf("%s on %s\n", argv[0], argv[1]);
	fflush(stdout);
	f = freopen("fanotify.log", "w+", stdout);
	if (f == NULL) {
		perror("freopen");
		exit(EXIT_FAILURE);
	}

	/* Create the file descriptor for accessing the fanotify API */
	fd = fanotify_init(FAN_CLOEXEC | FAN_CLASS_CONTENT | FAN_NONBLOCK,
					   O_RDONLY | O_LARGEFILE);
	if (fd == -1) {
		perror("fanotify_init");
		exit(EXIT_FAILURE);
	}

	/* Mark the mount for:
	   - permission events before opening files
	   - notification events after closing a write-enabled
		 file descriptor */
	if (fanotify_mark(fd, FAN_MARK_ADD | FAN_MARK_MOUNT,
			FAN_ACCESS | FAN_MODIFY | FAN_OPEN_PERM |
			FAN_CLOSE | FAN_OPEN | FAN_ACCESS_PERM |
			FAN_ONDIR | FAN_EVENT_ON_CHILD,
			-1, argv[1]) == -1) {

		perror("fanotify_mark");
		exit(EXIT_FAILURE);
	}

	/* Prepare for polling */
	nfds = 1;

	/* Fanotify input */
	fds[0].fd = fd;
	fds[0].events = POLLIN;

	/* This is the loop to wait for incoming events */
	printf("Listening for events.\n");
	while (1) {
		poll_num = poll(fds, nfds, -1);
		if (poll_num == -1) {
			if (errno == EINTR)     /* Interrupted by a signal */
				continue;           /* Restart poll() */

			perror("poll");         /* Unexpected error */
			exit(EXIT_FAILURE);
		}

		if (poll_num > 0) {

			if (fds[0].revents & POLLIN) {

				/* Fanotify events are available */
				handle_events(fd);
			}
		}
	}

	fclose(f);
	printf("Listening for events stopped.\n");
	exit(EXIT_SUCCESS);
}
