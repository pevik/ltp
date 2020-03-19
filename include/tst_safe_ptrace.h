// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2020 SUSE LLC <mdoucha@suse.cz>
 */
#ifndef TST_SAFE_PTRACE_H_
#define TST_SAFE_PTRACE_H_

#include <sys/ptrace.h>

/*
 * SAFE_PTRACE() treats any non-zero return value as error. Don't use it
 * for requests like PTRACE_PEEK* or PTRACE_SECCOMP_GET_FILTER which use
 * the return value to pass arbitrary data.
 */
long tst_safe_ptrace(const char *file, const int lineno,
	int req, pid_t pid, void *addr, void *data);
#define SAFE_PTRACE(req, pid, addr, data) \
	tst_safe_ptrace(__FILE__, __LINE__, req, pid, addr, data)

#endif /* TST_SAFE_PTRACE_H_ */
