// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) Huawei Technologies Co., Ltd., 2015
 * Copyright (C) 2022 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * When a process with non-zero user IDs performs an execve(), the
 * process's capability sets are cleared. When a process with zero
 * user IDs performs an execve(), the process's capability sets
 * are set.
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_LIBCAP
#define _GNU_SOURCE

#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/capability.h>

int main(int argc, char *argv[])
{
	FILE *f = NULL;
	cap_t caps;
	int i, last_cap;
	cap_flag_value_t flag_val;
	cap_flag_value_t expected_flag = 1;

	if (argc < 2) {
		printf("userns06_capcheck <privileged|unprivileged>\n");
		goto error;
	}

	f = fopen("/proc/sys/kernel/cap_last_cap", "r");
	if (f == NULL) {
		printf("fopen error: %s\n", strerror(errno));
		goto error;
	}

	if (!fscanf(f, "%d", &last_cap)) {
		printf("fscanf error: %s\n", strerror(errno));
		goto error;
	}

	if (strcmp("privileged", argv[1]))
		expected_flag = 0;

	caps = cap_get_proc();

	for (i = 0; i <= last_cap; i++) {
		cap_get_flag(caps, i, CAP_EFFECTIVE, &flag_val);
		if (flag_val != expected_flag)
			break;

		cap_get_flag(caps, i, CAP_PERMITTED, &flag_val);
		if (flag_val != expected_flag)
			break;
	}

	if (flag_val != expected_flag) {
		printf("unexpected effective/permitted caps at %d\n", i);
		goto error;
	}

	exit(0);

error:
	if (f)
		fclose(f);

	exit(1);
}

#else
int main(void)
{
	printf("System is missing libcap.\n");
	exit(1);
}
#endif
