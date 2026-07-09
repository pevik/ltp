# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) Linux Test Project, 2021-2026

top_srcdir		?= ../..
include $(top_srcdir)/include/mk/env_pre.mk

all:

test:
	@BUILDDIR="$(abs_builddir)" SRCDIR="$(abs_srcdir)" $(abs_srcdir)/test.sh
