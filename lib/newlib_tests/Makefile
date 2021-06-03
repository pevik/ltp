# SPDX-License-Identifier: GPL-2.0-or-later
top_srcdir		?= ../..

include $(top_srcdir)/include/mk/env_pre.mk

CFLAGS			+= -W -Wall
LDLIBS			+= -lltp

test08: CFLAGS+=-pthread
test09: CFLAGS+=-pthread
test15: CFLAGS+=-pthread
test16: CFLAGS+=-pthread
test16: LDLIBS+=-lrt
tst_expiration_timer: LDLIBS+=-lrt
tst_fuzzy_sync01: CFLAGS+=-pthread
tst_fuzzy_sync02: CFLAGS+=-pthread

ifeq ($(ANDROID),1)
FILTER_OUT_MAKE_TARGETS	+= test08
endif

TEST_TARGETS += test0[579] test1[2568] test_exec test_timer
TEST_TARGETS += tst_bool_expr tst_res_hexd tst_strstatus
TEST_TARGETS += tst_fuzzy_sync0[1-2]
TEST_TARGETS += shell/tst_check_driver.sh
TEST_TARGETS += shell/net/*.sh

include $(top_srcdir)/include/mk/generic_leaf_target.mk
