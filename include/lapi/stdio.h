// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2015 Cedric Hnyda <chnyda@suse.com>
 * Copyright (c) 2026 Petr Vorel <pvorel@suse.cz>
 */

#ifndef LAPI_STDIO_H__
#define LAPI_STDIO_H__

#include "config.h"
#include "lapi/syscalls.h"
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>

#ifndef HAVE_RENAMEAT2
int renameat2(int olddirfd, const char *oldpath, int newdirfd,
				const char *newpath, unsigned int flags)
{
	return tst_syscall(__NR_renameat2, olddirfd, oldpath, newdirfd,
						newpath, flags);
}
#endif

#endif /* LAPI_STDIO_H__ */
