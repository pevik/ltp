/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2021, BELLSOFT. All rights reserved.
 */

#ifndef TST_SCHED_H_
#define TST_SCHED_H_

#include <sched.h>

int tst_sched_setparam(pid_t pid, const struct sched_param *param);
int tst_sched_getparam(pid_t pid, struct sched_param *param);
int tst_sched_setscheduler(pid_t pid, int policy, const struct sched_param *param);
int tst_sched_getscheduler(pid_t pid);

#endif /* TST_SCHED_H_ */
