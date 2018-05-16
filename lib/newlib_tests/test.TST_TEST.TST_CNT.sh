#!/bin/sh
#
# Example test with tests in a single function
#

TST_TESTFUNC=do_test
TST_CNT=2
. tst_test.sh

do_test()
{
	case $1 in
	1) tst_res TPASS "Test $1 passed with no data ('$2')";;
	2) tst_res TPASS "Test $1 passed with no data ('$2')";;
	esac
}

tst_run
