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

#ifdef HAVE_LIBCAP
# define TST_NO_DEFAULT_MAIN
#endif

#include "tst_test.h"

#ifdef HAVE_LIBCAP

#include <string.h>
#include <sys/wait.h>
#include <sys/capability.h>

int main(int argc, char *argv[])
{
	cap_t caps;
	int i, last_cap;
	cap_flag_value_t flag_val;
	cap_flag_value_t expected_flag = 1;

	if (argc < 2)
		tst_brk(TBROK, "userns06_capcheck <privileged|unprivileged>");

	tst_reinit();

	SAFE_FILE_SCANF("/proc/sys/kernel/cap_last_cap", "%d", &last_cap);

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

	if (flag_val != expected_flag)
		tst_res(TFAIL, "unexpected effective/permitted caps at %d: %d (%d)",
			i, flag_val, expected_flag);
	else
		tst_res(TPASS, "expected caps at %d", i);
}

#else
TST_TEST_TCONF("System is missing libcap");
#endif
