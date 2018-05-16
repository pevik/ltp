#!/bin/sh
#
# This is a basic test for true shell buildin
#

TST_TESTFUNC=do_test
. tst_test.sh

do_test()
{
	true
	ret=$?

	tst_res TINFO "Test $1 passed with no data ('$2')"

	if [ $ret -eq 0 ]; then
		tst_res TPASS "true returned 0"
	else
		tst_res TFAIL "true returned $ret"
	fi
}

tst_run
