#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2022 Petr Vorel <pvorel@suse.cz>

TST_TESTFUNC="test"
TST_NEEDS_TMPDIR=1
TST_CNT=3

test1()
{
	ROD cd /
	EXPECT_PASS [ "$PWD" != "/" ]
}

test2()
{
	ROD echo foo > /nonexisting-file
	tst_res TPASS "ROD redirecting without qoting continue on failure. NOTE: proper syntax needs to escape '>': ROD echo foo \> /nonexisting-file"
	EXPECT_FAIL 'cat /nonexisting-file'
}

test3()
{
	ROD echo foo \> file
	tst_res TPASS "ROD redirect quoting syntax works"
	EXPECT_PASS '[ $(cat file) = foo ]'
}

. tst_test.sh
tst_run
