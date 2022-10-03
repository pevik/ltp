// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2010 Hajime Taira <htaira@redhat.com>
 * Copyright (c) 2010 Masatake Yamato <yamato@redhat.com>
 * Copyright (c) 2022 Petr Vorel <pvorel@suse.cz>
 */

#include <errno.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

#define err_exit(...) ({ \
	fprintf(stderr, __VA_ARGS__); \
	if (errno) \
		fprintf(stderr, ": %s (%d)", strerror(errno), errno); \
	fprintf(stderr, "\n"); \
	exit(EXIT_FAILURE); \
})

int main(int argc, char *argv[])
{
	int fd;
	struct stat sb;

	if (argc < 2)
		err_exit("USAGE: df01_fsfreeze <mountpoint>");

	fd = open(argv[1], O_RDONLY);
	if (fd < 0)
		err_exit("open '%s' failed", argv[1]);

	if (fstat(fd, &sb) == -1)
		err_exit("stat of '%s' failed", argv[1]);

	if (!S_ISDIR(sb.st_mode))
		err_exit("%s: is not a directory", argv[1]);

	if (ioctl(fd, FIFREEZE, 0) < 0)
		err_exit("ioctl FIFREEZE on '%s' failed", argv[1]);

	if (ioctl(fd, FITHAW, 0) < 0)
		err_exit("ioctl FITHAW on '%s' failed", argv[1]);

	close(fd);

	return EXIT_SUCCESS;
}
