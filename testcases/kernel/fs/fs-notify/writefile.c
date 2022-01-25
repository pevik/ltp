#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char **argv)
{
	int fd, ret;
	char buf[BUFSIZ];

	memset(buf, 1, BUFSIZ);
	while (1) {
		fd = open(argv[1], O_RDWR);
		if (fd == -1) {
			perror("writefile open");
			exit(EXIT_FAILURE);
		}
		ret = write(fd, buf, BUFSIZ);
		if (ret == -1)
			perror("writefile write");
		usleep(1);
	}
	close(fd);
	return 0;
}
