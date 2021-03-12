/*
 * Copyright (c) 2002, Intel Corporation. All rights reserved.
 * Copyright (c) 2012, Cyril Hrubis <chrubis@suse.cz>
 *
 * This file is licensed under the GPL license.  For the full content
 * of this license, see the COPYING file at the top level of this
 * source tree.
 *
 * The mmap() function shall fail if:
 * [ENOMEM] MAP_FIXED was specified, and the range [addr,addr+len)
 * exceeds that allowed for the address space of a process;
 * or, if MAP_FIXED was not specified and
 * there is insufficient room in the address space to effect the mapping.
 *
 * Test Steps:
 * 1. In a very long loop, keep mapping a shared memory object,
 *    until there this insufficient room in the address space;
 * 3. Should get ENOMEM.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include "posixtest.h"

int main(void)
{
	void *pa;
	size_t len;
	int fd;

	/* Size of the shared memory object */
	size_t shm_size = 1024;

	size_t mapped_size = 0;

	printf("/proc/self/maps before testing\n");
	system("cat /proc/self/maps");

	len = shm_size;

	mapped_size = 0;
	while (mapped_size < SIZE_MAX) {
		pa = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
		if (pa == MAP_FAILED && errno == ENOMEM) {
			printf("Total mapped size is %lu bytes\n",
			       (unsigned long)mapped_size);
			printf("Test PASSED\n");

			printf("/proc/self/maps after testing\n");
			system("cat /proc/self/maps");

			return PTS_PASS;
		}

		mapped_size += shm_size;
		if (pa == MAP_FAILED)
			perror("Error at mmap()");
	}

	close(fd);
	printf("Test FAILED: Did not get ENOMEM as expected\n");
	return PTS_FAIL;
}
