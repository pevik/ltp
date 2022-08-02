#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2022 Petr Vorel <pvorel@suse.cz>

TST_MOUNT_DEVICE=1
TST_FS_TYPE=ext4
TST_TESTFUNC=test
TST_CNT=3

test1()
{
	EXPECT_PASS "cd $TST_MNTPOINT"
}

test2()
{
	EXPECT_PASS "grep '$TST_MNTPOINT $TST_FS_TYPE' /proc/mounts"
}

test3()
{
	tst_brk TCONF "quit with TCONF to make sure tst_device don't get confused"
}

. tst_test.sh
tst_run
