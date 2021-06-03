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

# C API tests
# NOTE: only those which exit 0 (TPASS) or 32 (TCONF)
TEST_TARGETS += test05 test07 test09 test12 test15 test16 test18
TEST_TARGETS += test_exec test_timer tst_bool_expr tst_res_hexd
TEST_TARGETS += tst_strstatus tst_fuzzy_sync01 tst_fuzzy_sync02

# shell API tests
# NOTE: only those which exit 0 (TPASS) or 32 (TCONF)
TEST_SHELL_TARGETS += shell/tst_check_driver.sh
TEST_SHELL_TARGETS += shell/net/*.sh

include $(top_srcdir)/include/mk/generic_leaf_target.mk
