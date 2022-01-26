/*
 * Copyright (c) International Business Machines Corp., 2009
 * Some wrappers for clone functionality.  Thrown together by Serge Hallyn
 * <serue@us.ibm.com> based on existing clone usage in ltp.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See
 * the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sched.h>
#include <stdarg.h>
#include "config.h"
#include "tst_clone.h"

#undef clone			/* we want to use clone() */

int ltp_clone(unsigned long flags, int (*fn)(void *arg), void *arg,
              size_t stack_size, void *stack)
{
	return clone(fn, stack, flags, arg);
}

int ltp_clone7(unsigned long flags, int (*fn)(void *arg), void *arg,
               size_t stack_size, void *stack, ...)
{
	pid_t *ptid, *ctid;
	void *tls;
	va_list arg_clone;

	va_start(arg_clone, stack);
	ptid = va_arg(arg_clone, pid_t *);
	tls = va_arg(arg_clone, void *);
	ctid = va_arg(arg_clone, pid_t *);
	va_end(arg_clone);

	return clone(fn, stack, flags, arg, ptid, tls, ctid);
}

/*
 * ltp_alloc_stack: allocate stack of size 'size', that is sufficiently
 * aligned for all arches. User is responsible for freeing allocated
 * memory.
 * Returns pointer to new stack. On error, returns NULL with errno set.
 */
void *ltp_alloc_stack(size_t size)
{
	void *ret = NULL;
	int err;

	err = posix_memalign(&ret, 64, size);
	if (err)
		errno = err;

	return ret;
}

/*
 * ltp_clone_alloc: also does the memory allocation for clone with a
 * caller-specified size.
 */
int
ltp_clone_alloc(unsigned long clone_flags, int (*fn) (void *arg), void *arg,
		 size_t stack_size)
{
	void *stack;
	int ret;
	int saved_errno;

	stack = ltp_alloc_stack(stack_size);
	if (stack == NULL)
		return -1;

	ret = ltp_clone(clone_flags, fn, arg, stack_size, stack);

	if (ret == -1) {
		saved_errno = errno;
		free(stack);
		errno = saved_errno;
	}

	return ret;
}

/*
 * ltp_clone_quick: calls ltp_clone_alloc with predetermined stack size.
 * Experience thus far suggests that one page is often insufficient,
 * while 6*getpagesize() seems adequate.
 */
int ltp_clone_quick(unsigned long clone_flags, int (*fn) (void *arg), void *arg)
{
	size_t stack_size = getpagesize() * 6;

	return ltp_clone_alloc(clone_flags, fn, arg, stack_size);
}
