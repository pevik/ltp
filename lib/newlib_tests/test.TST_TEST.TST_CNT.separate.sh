#!/bin/sh
#
# Example test with tests in separate functions
#

TST_TESTFUNC=test
TST_CNT=2
. tst_test.sh

test1()
{
	tst_res TPASS "Test $1 passed with no data ('$2')"
}

test2()
{
	tst_res TPASS "Test $1 passed with no data ('$2')"
}

tst_run
