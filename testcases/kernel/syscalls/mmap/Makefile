# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) International Business Machines  Corp., 2001

top_srcdir		?= ../../../..

include $(top_srcdir)/include/mk/testcases.mk

include $(top_srcdir)/include/mk/generic_leaf_target.mk

LDLIBS 			+= -lpthread
#
# We use recursive calls to to grow the stack, to test the
# MAP_GROSWDOWN flag. But tail call optimization by the compiler
# can prevent the recusive call and stack growth. Disable tail
# call optmization using -fno-optimize-sibling-calls
#
mmap18: CFLAGS          += -fno-optimize-sibling-calls
