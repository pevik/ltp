#!/bin/sh
# Copyright (c) 2021 Petr Vorel <pvorel@suse.cz>

LTP_C_API_TESTS="${LTP_C_API_TESTS:-test05 test07 test09 test12 test15 test16 test18
test_exec test_timer tst_bool_expr tst_res_hexd tst_strstatus tst_fuzzy_sync02}"

LTP_SHELL_API_TESTS="${LTP_SHELL_API_TESTS:-shell/tst_check_driver.sh shell/net/*.sh}"

cd $(dirname $0)
PATH="$PWD/../../testcases/lib/:$PATH"

. tst_ansi_color.sh

usage()
{
	echo "Usage: $0 [-c|-s]"
	echo "-c    run C API tests only"
	echo "-s    run shell API tests only"
	echo "-h    print this help"
}

# custom version
tst_flag2mask()
{
	case "$1" in
	TPASS) return 0;;
	TFAIL) return 1;;
	TBROK) return 2;;
	TWARN) return 4;;
	TINFO) return 16;;
	TCONF) return 32;;
	esac
}

# custom version
tst_res()
{
	if [ $# -eq 0 ]; then
		echo >&2
		return
	fi

	local res="$1"
	shift

	tst_color_enabled
	local color=$?

	printf "runtest " >&2
	tst_print_colored $res "$res: " >&2
	echo "$@" >&2

}

# custom version
tst_brk()
{
	local res="$1"
	shift

	tst_res
	tst_res $res $@

	exit $(tst_flag2mask $res)
}

run_tests()
{
	local target="$1"
	local i ret tconf tpass vars

	eval vars="\$LTP_${target}_API_TESTS"

	tst_res TINFO "=== Run $target tests ==="

	for i in $vars; do
		tst_res TINFO "* $i"
		./$i
		ret=$?

		case $ret in
			0) tpass="$tpass $i";;
			1) tst_brk TFAIL "$i failed with TFAIL";;
			2) tst_brk TFAIL "$i failed with TBROK";;
			4) tst_brk TFAIL "$i failed with TWARN";;
			32) tconf="$tconf $i";;
		esac
		tst_res
	done

	[ -z "$tpass" ] && tpass=" none"
	[ -z "$tconf" ] && tconf=" none"

	tst_res TINFO "=== $target TEST RESULTS ==="
	tst_res TINFO "Tests exited with TPASS:$tpass"
	tst_res TINFO "Tests exited with TCONF:$tconf"
	tst_res
}

run=
while getopts chs opt; do
	case $opt in
		'h') usage; exit 0;;
		'c') run="c";;
		's') run="s";;
		*) usage; tst_brk TBROK "Error: invalid option";;
	esac
done

tst_res TINFO "PATH='$PATH'"

if [ -z "$run" -o "$run" = "c" ]; then
	run_tests "C"
fi

if [ -z "$run" -o "$run" = "s" ]; then
	run_tests "SHELL"
fi

tst_res TPASS "No test failed"
