#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2021 FUJITSU LIMITED. All rights reserved.

TST_TESTFUNC=do_test
TST_NEEDS_CMDS="tst_check_kconfigs"
TST_NEEDS_KCONFIGS="CONFIG_EXT4"
TST_TCONF_IF_KCONFIG=0

. tst_test.sh

do_test()
{
	tst_res TPASS "Kernel .config doesn't have CONFIG_EXT4!"
}

tst_run
