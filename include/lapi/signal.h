/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2022 SUSE LLC
 */
#include <signal.h>

const int tst_sigrtmin_kern = 32;
const int tst_sigrtmax_kern = 64;

/* Indicates that libc has reserved a RT signal for use by the system
 * libraries. Usually threading, but could be anything. */
__attribute__((warn_unused_result))
static inline int tst_signal_is_reserved_rt(const int sig)
{
	return 	sig >= tst_sigrtmin_kern && sig < SIGRTMIN;
}

/* Indicates that a signal is in the kernel's realtime set. This
 * includes signals reserved by libc. */
__attribute__((const, warn_unused_result))
static inline int tst_signal_is_kern_rt(const int sig)
{
	return 	sig >= tst_sigrtmin_kern && sig < tst_sigrtmax_kern;
}
