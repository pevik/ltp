# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2020 Cyril Hrubis <chrubis@suse.cz>

top_srcdir		?= ..

include $(top_srcdir)/include/mk/testcases.mk

MAKE_TARGETS		:= ltp.json
HOST_MAKE_TARGETS	:= metaparse metaparse-sh
INSTALL_DIR		= metadata

metaparse: HOST_CFLAGS		+= -I$(abs_srcdir)/../include -L$(abs_builddir)/../lib
metaparse: HOST_LDLIBS		+= -lltp

.PHONY: ltp.json

ltp.json: metaparse metaparse-sh
	$(abs_srcdir)/parse.sh > ltp.json

test: metaparse metaparse-sh
	METAPARSEDIR=$$METAPARSEDIR $(MAKE) -C tests/ test

include $(top_srcdir)/include/mk/generic_leaf_target.mk
