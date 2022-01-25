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
	int wd;

	while (1) {
		notify_fd = inotify_init1(IN_CLOEXEC);
		if (notify_fd == -1)
			perror("inotify_init1");
		close(notify_fd);
	}
	return 0;
}
