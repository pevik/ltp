#!/bin/sh
# Copyright (c) 2021 Petr Vorel <pvorel@suse.cz>

LTP_C_API_TESTS="${LTP_C_API_TESTS:-test05 test07 test09 test12 test15 test18
test_exec test_timer tst_res_hexd tst_strstatus tst_fuzzy_sync01
tst_fuzzy_sync02 tst_fuzzy_sync03}"

LTP_SHELL_API_TESTS="${LTP_SHELL_API_TESTS:-shell/tst_check_driver.sh shell/net/*.sh}"

cd $(dirname $0)
PATH="$PWD/../../testcases/lib/:$PATH"

. tst_ansi_color.sh

usage()
{
	cat << EOF
Usage: $0 [-b DIR ] [-c|-s]
-b DIR  build directory (required for out-of-tree build)
-c      run C API tests only
-s      run shell API tests only
-h      print this help
EOF
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

	tst_flag2mask "$res"
	local mask=$?

	tst_res
	tst_res $res $@

	exit $mask
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
			127) tst_brk TBROK "Error: file not found (wrong PATH? out-of-tree build without -b?), exit code: $ret";;
			*) tst_brk TBROK "Error: unknown failure, exit code: $ret";;
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

run_c_tests()
{
	if [ "$builddir" ]; then
		cd $builddir/lib/newlib_tests
	fi

	run_tests "C"

	if [ "$builddir" ]; then
		cd -
	fi
}

run_shell_tests()
{
	run_tests "SHELL"
}

builddir=
run=
while getopts b:chs opt; do
	case $opt in
		'h') usage; exit 0;;
		'b') builddir=$OPTARG; PATH="$builddir/testcases/lib:$PATH";;
		'c') run="c";;
		's') run="s";;
		*) usage; tst_brk TBROK "Error: invalid option";;
	esac
done

tst_res TINFO "PATH='$PATH'"

if [ -z "$run" -o "$run" = "c" ]; then
	run_c_tests
fi

if [ -z "$run" -o "$run" = "s" ]; then
	run_shell_tests
fi

tst_res TPASS "No test failed"
