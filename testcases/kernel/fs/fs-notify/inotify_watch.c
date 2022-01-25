#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdio.h>
#include <sys/inotify.h>

int main(int argc, char *argv[])
{
	int notify_fd;
	int wd, ret;
	char *buf;
	FILE *f;

	f = freopen("inotify.log", "w+", stdout);
	if (f == NULL) {
		perror("freopen");
		exit(EXIT_FAILURE);
	}

	buf = malloc(sizeof(struct inotify_event) + NAME_MAX + 1);
	if (buf == NULL) {
		perror("malloc");
		return -1;
	}

	notify_fd = inotify_init1(IN_CLOEXEC);
	if (notify_fd == -1) {
		perror("inotify_init1");
		return -1;
	}

	wd = inotify_add_watch(notify_fd, argv[1],
		IN_ACCESS | IN_ATTRIB | IN_CLOSE_WRITE | IN_CLOSE_NOWRITE |
		IN_CREATE | IN_DELETE | IN_DELETE_SELF | IN_MODIFY |
		IN_MOVE_SELF | IN_MOVED_FROM | IN_MOVED_TO | IN_OPEN);

	if (wd < 0) {
		perror("inotify_add_watch");
		return -1;
	}

	while (ret = read(notify_fd, buf, NAME_MAX) != -1) {
		struct inotify_event *ip = (struct inotify_event *)buf;
		printf("name %s event ", ip->name);
		switch (ip->mask) {
		case IN_ACCESS:
			printf("access\n");
			break;
		case IN_ATTRIB:
			printf("attrib\n");
			break;
		case IN_CLOSE_WRITE:
			printf("close write\n");
			break;
		case IN_CLOSE_NOWRITE:
			printf("close nowrite\n");
			break;
		case IN_CREATE:
			printf("create\n");
			break;
		case IN_DELETE:
			printf("delete\n");
			break;
		case IN_DELETE_SELF:
			printf("deleteself\n");
			break;
		case IN_MODIFY:
			printf("modify\n");
			break;
		case IN_MOVE_SELF:
			printf("move self\n");
			break;
		case IN_MOVED_FROM:
			printf("move from\n");
			break;
		case IN_MOVED_TO:
			printf("move to\n");
			break;
		case IN_OPEN:
			printf("open\n");
			break;
		default:
			printf("\n");
			break;
		};
	}

	ret = inotify_rm_watch(notify_fd, wd);
	if (ret < 0)
		perror("inotify_rm_watch");

	fclose(f);
	close(notify_fd);
	return 0;
}
