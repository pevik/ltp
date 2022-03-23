#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2022 Petr Vorel <pvorel@suse.cz>

TST_MOUNT_DEVICE=1
TST_FS_TYPE=ext4
TST_TESTFUNC=test
TST_CNT=2
. tst_test.sh

test1()
{
	tst_res TPASS "device formatted"
}

test2()
{
	EXPECT_PASS "grep '$TST_MNTPOINT $TST_FS_TYPE' /proc/mounts"
}

tst_run
