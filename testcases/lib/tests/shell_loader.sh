#!/bin/sh
#
# needs_tmpdir=1

. tst_env.sh

case "$PWD" in
	/tmp/*)
	tst_res TPASS "We are running in temp directory in $PWD";;
	*)
	tst_res TFAIL "We are not running in temp directory but $PWD";;
esac
