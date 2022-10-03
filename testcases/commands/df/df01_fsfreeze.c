// SPDX-License-Identifier: GPL-2.0-or-later

#include <fcntl.h>
#include <linux/fs.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define err_exit(...) ({ \
	fprintf(stderr, __VA_ARGS__); \
	fprintf(stderr, "\n"); \
	exit(2); \
})

int main(int argc, char *argv[])
{
	int fd;

	if (argc < 2)
		err_exit("USAGE: df01_fsfreeze <mountpoint>");

	fd = open(argv[1], O_RDONLY);
	if (fd < 0)
		err_exit("open '%s' failed", argv[1]);

	if (ioctl(fd, FIFREEZE, 0) < 0)
		err_exit("ioctl FIFREEZE on '%s' failed", argv[1]);

	usleep(100);

	if (ioctl(fd, FITHAW, 0) < 0)
		err_exit("ioctl FITHAW on '%s' failed", argv[1]);

	close(fd);

	return 0;
}
