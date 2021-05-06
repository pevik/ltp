// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2021 Joerg Vehlow <joerg.vehlow@aox-tech.de>
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#define TST_NO_DEFAULT_MAIN

#include "tst_test.h"

static void print_help(void)
{
	printf("Usage: tst_timeout_kill interval pid\n\n");
	printf("       Terminates process group pid after interval seconds\n");
}

static int kill_safe(__pid_t pid, int signal) {
	int rc;

	rc = kill(pid, signal);
	if (rc == -1) {
		/* The process terminated already */
		if (errno == ESRCH)
			return ESRCH;
		tst_brk(TBROK, "Unable to send signals to process '%d'\n", pid);
	}
	return 0;
}

static int do_timeout(int interval, __pid_t pid)
{
	int i;

	sleep(interval);

	tst_res(TWARN, "Test timed out, sending SIGINT! If you are running on slow machine, try exporting LTP_TIMEOUT_MUL > 1");

	if (kill_safe(-pid, SIGINT) == ESRCH)
		return 0;

	usleep(100000);

	for (i = 0; i < 10; ++i) {
		if (kill_safe(pid, 0) == ESRCH)
			return 0;
		tst_res(TWARN, "Test is still running, waiting %ds", 10 - i);
		sleep(1);
	}

	tst_res(TWARN, "Test still running, sending SIGKILL");
	kill(-pid, SIGKILL);
	return 0;
}

int main(int argc, char *argv[])
{
	int opt;
	unsigned long interval, pid;
	char *end;

	while ((opt = getopt(argc, argv, ":h")) != -1) {
		switch (opt) {
		case 'h':
			print_help();
			return 0;
		default:
			print_help();
			return 1;
		}
	}

	if (optind >= argc - 1) {
		fprintf(stderr, "ERROR: Expected interval and pid argument\n\n");
		print_help();
		return 1;
	}

	interval = strtoul(argv[optind], &end, 10);
	if (end != argv[optind] + strlen(argv[optind])) {
		fprintf(stderr, "ERROR: Invalid interval '%s'\n\n", argv[optind]);
		print_help();
		return 1;
	}
	optind++;

	pid = strtol(argv[optind], &end, 10);
	if (end != argv[optind] + strlen(argv[optind])) {
		fprintf(stderr, "ERROR: Invalid pid '%s'\n\n", argv[optind]);
		print_help();
		return 1;
	}

	if (kill_safe(pid, 0) == ESRCH) {
		fprintf(stderr, "ERROR: Process with pid '%ld' does not exist\n", pid);
		return 1;
	}

	return do_timeout(interval, pid);
}
