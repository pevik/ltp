# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2019 Cyril Hrubis <chrubis@suse.cz>
# Copyright (c) 2020 Petr Vorel <pvorel@suse.cz>

top_srcdir		?= ../..

include $(top_srcdir)/include/mk/env_pre.mk
include $(top_srcdir)/include/mk/functions.mk

HOST_MAKE_TARGETS	:= main
HOST_LDFLAGS 		+= -lclang

include $(top_srcdir)/include/mk/generic_leaf_target.mk
