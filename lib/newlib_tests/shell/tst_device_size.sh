#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2024 Petr Vorel <pvorel@suse.cz>

TST_NEEDS_DEVICE=1
TST_DEVICE_SIZE=10
TST_TESTFUNC=test

test()
{
	tst_res TPASS "TST_DEVICE_SIZE=$TST_DEVICE_SIZE"
	EXPECT_PASS "du -ms . | grep -qw $TST_DEVICE_SIZE"
}

. tst_test.sh
tst_run
