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

	notify_fd = inotify_init1(IN_CLOEXEC);
	if (notify_fd == -1) {
		perror("inotify_init1");
		return -1;
	}

	while (1) {
		wd = inotify_add_watch(notify_fd, argv[1],
			IN_ACCESS | IN_ATTRIB | IN_CLOSE_WRITE |
			IN_CLOSE_NOWRITE | IN_CREATE | IN_DELETE |
			IN_DELETE_SELF | IN_MODIFY | IN_MOVE_SELF |
			IN_MOVED_FROM | IN_MOVED_TO | IN_OPEN);
		if (wd < 0)
			perror("inotify_add_watch");

		ret = inotify_rm_watch(notify_fd, wd);
		if (ret < 0)
			perror("inotify_rm_watch");
	}
	close(notify_fd);
	return 0;
}
