// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2015 Linux Test Project
 */

#ifndef LAPI_GETRANDOM_H__
#define LAPI_GETRANDOM_H__

#include "config.h"

#ifdef HAVE_SYS_RANDOM_H
# include <sys/random.h>
# if !defined(SYS_getrandom) && defined(__NR_getrandom)
   /* usable kernel-headers, but old glibc-headers */
#  define SYS_getrandom __NR_getrandom
# endif
#elif HAVE_LINUX_RANDOM_H
# include <linux/random.h>
#endif

/*
 * Flags for getrandom(2)
 *
 * GRND_NONBLOCK	Don't block and return EAGAIN instead
 * GRND_RANDOM		Use the /dev/random pool instead of /dev/urandom
 */

#ifndef GRND_NONBLOCK
# define GRND_NONBLOCK	0x0001
#endif

#ifndef GRND_RANDOM
# define GRND_RANDOM	0x0002
#endif

#if !defined(HAVE_SYS_RANDOM_H) && defined(SYS_getrandom)
/* libc without function, but we have syscall */
static int getrandom(void *buf, size_t buflen, unsigned int flags)
{
	return (syscall(SYS_getrandom, buf, buflen, flags));
}
# define HAVE_GETRANDOM
#endif

#endif /* LAPI_GETRANDOM_H__ */
