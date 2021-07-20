// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2021, BELLSOFT. All rights reserved.
 */

#include <errno.h>
#include <unistd.h>
#include "tst_sched.h"
#include "lapi/syscalls.h"

#define TST_SCHED_COMMON(SCALL, ...) do {			\
	int ret = SCALL(__VA_ARGS__);				\
	if (ret == -1 && errno == ENOSYS)			\
		return syscall(__NR_##SCALL, __VA_ARGS__);	\
	return ret;						\
} while (0)

int tst_sched_setparam(pid_t pid, const struct sched_param *param)
{
	TST_SCHED_COMMON(sched_setparam, pid, param);
}

int tst_sched_getparam(pid_t pid, struct sched_param *param)
{
	TST_SCHED_COMMON(sched_getparam, pid, param);
}

int tst_sched_setscheduler(pid_t pid, int policy, const struct sched_param *param)
{
	TST_SCHED_COMMON(sched_setscheduler, pid, policy, param);
}

int tst_sched_getscheduler(pid_t pid)
{
	TST_SCHED_COMMON(sched_getscheduler, pid);
}
