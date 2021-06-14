# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2021 SUSE LLC <rpalethorpe@suse.com>
# Copyright (c) 2019 Cyril Hrubis <chrubis@suse.cz>
# Copyright (c) 2020 Petr Vorel <pvorel@suse.cz>

top_srcdir		?= ../..

include $(top_srcdir)/include/mk/env_pre.mk
include $(top_srcdir)/include/mk/functions.mk

HOST_MAKE_TARGETS	:= main
ifneq ($(strip $(LLVM_CONFIG)),":")
HOST_CFLAGS		+= $(shell $(LLVM_CONFIG) --cflags)
HOST_LDFLAGS		+= $(shell $(LLVM_CONFIG) --ldflags)
endif
HOST_LDLIBS 		+= $(CLANG_LIBS)

.PHONY: check-clang-deps
check-clang-deps:
ifeq ($(strip $(CLANG)),":")
	$(warning clang-check: clang executable is missing which we use to find the Clang resource directory; install clang package)
endif
ifeq ($(strip $(LLVM_CONFIG)),":")
	$(warning clang-check: llvm-config is missing which we use to find the libclang location; install the clang/LLVM devel packages)
endif
ifeq ($(strip $(CLANG_LIBS)),)
	$(warning clang-check: libclang missing)
endif
ifneq ($(strip $(HAVE_CLANG_C_INDEX_H)),1)
	$(warning clang-check: clang development headers are missing; install the clang devel package)
endif

MAKE_DEPS += check-clang-deps

include $(top_srcdir)/include/mk/generic_leaf_target.mk
