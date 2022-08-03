#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2022 Petr Vorel <pvorel@suse.cz>

TST_ALL_FILESYSTEMS=1
TST_TESTFUNC=test
TST_CNT=2

test1()
{
	tst_res TINFO "pev: start test1" # FIXME: debug
	tst_res TPASS "device using filesystem"
}

test2()
{
	local pattern


	tst_res TINFO "pev: start test2" # FIXME: debug

	if [ "$TST_FS_TYPE" = "exfat" -o "$TST_FS_TYPE" = "ntfs" ]; then
		pattern="|fuseblk"
	fi

	EXPECT_PASS "grep -E '$TST_MNTPOINT ($TST_FS_TYPE${pattern})' /proc/mounts"
}

. tst_test.sh
tst_run
