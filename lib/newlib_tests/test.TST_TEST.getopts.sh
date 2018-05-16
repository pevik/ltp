#!/bin/sh
#
# Optional test command line parameters
#

TST_OPTS="af:"
TST_USAGE=usage
TST_PARSE_ARGS=parse_args
TST_TESTFUNC=do_test

. tst_test.sh

ALTERNATIVE=0
MODE="foo"

usage()
{
	cat << EOF
usage: $0 [-a] [-f <foo|bar>]

OPTIONS
-a     Enable support for alternative foo
-f     Specify foo or bar mode
EOF
}

parse_args()
{
	case $1 in
	a) ALTERNATIVE=1;;
	f) MODE="$2";;
	esac
}

do_test()
{
	tst_res TPASS "Test $1 passed with data '$2': a: '$ALTERNATIVE', f: '$MODE'"
}

tst_run
