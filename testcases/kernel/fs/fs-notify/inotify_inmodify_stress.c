#include <sys/inotify.h>
#include <stdio.h>
#include <unistd.h>

/*
 * Calls inotify_rm_watch in a loop.
 */
int main(int argc, char **argv)
{
	int fd = inotify_init();
	while (1) {
		int wd = inotify_add_watch(fd, argv[1], IN_MODIFY);
		inotify_rm_watch(fd, wd);
	}
	close(fd);
	return 0;
}
