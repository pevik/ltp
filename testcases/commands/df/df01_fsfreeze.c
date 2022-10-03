// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/fs.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define TST_NO_DEFAULT_MAIN
#include "tst_test.h"

extern struct tst_test *tst_test;

static struct tst_test test = {
};

int main(int argc, char *argv[])
{
	int fd;

	/* force messages to be printed from new library */
	tst_test = &test;

	if (argc < 2) {
		fprintf(stderr, "USAGE: df01_fsfreeze <mountpoint>");
		exit(1);
	}

	fd = SAFE_OPEN(argv[1], O_RDONLY);
	SAFE_IOCTL(fd, FIFREEZE, 0);
	usleep(100);
	SAFE_IOCTL(fd, FITHAW, 0);

	return 0;
}
