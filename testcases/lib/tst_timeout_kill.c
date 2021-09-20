// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2021 Cyril Hrubis <chrubis@suse.cz>
 */

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <tst_res_flags.h>
#include <tst_ansi_color.h>

static void print_help(const char *name)
{
	fprintf(stderr, "usage: %s timeout pid\n", name);
}

static void print_msg(int type, const char *msg)
{
	const char *strtype = "";

	switch (type) {
	case TBROK:
		strtype = "TBROK";
	break;
	case TINFO:
		strtype = "TINFO";
	break;
	}

	if (tst_color_enabled(STDERR_FILENO)) {
		fprintf(stderr, "%s%s: %s%s\n", tst_ttype2color(type),
			strtype, ANSI_COLOR_RESET, msg);
	} else {
		fprintf(stderr, "%s: %s\n", strtype, msg);
	}
}

int main(int argc, char *argv[])
{
	int timeout, pid, ret, i;

	if (argc != 3) {
		print_help(argv[0]);
		return 1;
	}

	timeout = atoi(argv[1]);
	pid = atoi(argv[2]);

	if (timeout < 0) {
		fprintf(stderr, "Invalid timeout '%s'", argv[1]);
		print_help(argv[0]);
		return 1;
	}

	if (pid <= 1) {
		fprintf(stderr, "Invalid pid '%s'", argv[2]);
		print_help(argv[0]);
		return 1;
	}

	if (timeout)
		sleep(timeout);

	print_msg(TBROK, "Test timed out, sending SIGTERM! If you are running on slow machine, try exporting LTP_TIMEOUT_MUL > 1");

	ret = kill(-pid, SIGTERM);
	if (ret) {
		print_msg(TBROK, "kill() failed");
		return 1;
	}

	usleep(100000);

	i = 10;

	while (!kill(-pid, 0) && i-- > 0) {
		print_msg(TINFO, "Test is still running...");
		sleep(1);
	}

	if (!kill(-pid, 0)) {
		print_msg(TBROK, "Test is still running, sending SIGKILL");
		ret = kill(-pid, SIGKILL);
		if (ret) {
			print_msg(TBROK, "kill() failed");
			return 1;
		}
	}

	return 0;
}
