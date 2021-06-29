# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2021 SUSE LLC <rpalethorpe@suse.com>

top_srcdir		?= ../..

include $(top_srcdir)/include/mk/env_pre.mk
include $(top_srcdir)/include/mk/functions.mk

SPARSE_SRC	?= sparse-src

$(SPARSE_SRC)/libsparse.a:
	$(error "You need to build Sparse. See tools/sparse/README.md")

HOST_MAKE_TARGETS	:= main
MAKE_DEPS		+= $(SPARSE_SRC)/libsparse.a
HOST_CFLAGS		+= -I$(SPARSE_SRC)
HOST_LDLIBS		+= $(SPARSE_SRC)/libsparse.a


include $(top_srcdir)/include/mk/generic_leaf_target.mk
