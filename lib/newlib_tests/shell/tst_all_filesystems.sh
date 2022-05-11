#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2022 Petr Vorel <pvorel@suse.cz>

TST_ALL_FILESYSTEMS=1
TST_TESTFUNC=test
TST_CNT=2

test1()
{
	EXPECT_PASS "cd $TST_MNTPOINT"
}

test2()
{
	local pattern


	if [ "$TST_FS_TYPE" = "exfat" -o "$TST_FS_TYPE" = "ntfs" ]; then
		pattern="|fuseblk"
	fi

	EXPECT_PASS "grep -E '$TST_MNTPOINT ($TST_FS_TYPE${pattern})' /proc/mounts"
}

. tst_test.sh
tst_run
