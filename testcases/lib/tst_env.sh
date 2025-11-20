#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2024-2025 Cyril Hrubis <chrubis@suse.cz>
# Copyright (c) Linux Test Project, 2025
#
# This is a minimal test environment for a shell scripts executed from C by
# tst_run_shell() function. Shell tests must use the tst_loader.sh instead!
#

tst_script_name=$(basename $0)

# bash does not expand aliases in non-iteractive mode, enable it
if [ -n "$BASH_VERSION" ]; then
	shopt -s expand_aliases
fi

# dash does not support line numbers even though this is mandated by POSIX
if [ -z "$LINENO" ]; then
	LINENO=-1
fi

tst_brk_()
{
	tst_res_ "$@"

	case "$3" in
		"TBROK") exit 2;;
		*) exit 0;;
	esac
}

alias tst_res="tst_res_ $tst_script_name \$LINENO"
alias tst_brk="tst_brk_ $tst_script_name \$LINENO"

ROD_SILENT()
{
	local tst_out

	tst_out=$(tst_rod "$@" 2>&1)
	if [ $? -ne 0 ]; then
		echo "$tst_out"
		tst_brk TBROK "$@ failed"
	fi
}

ROD()
{
	tst_rod "$@"
	if [ $? -ne 0 ]; then
		tst_brk TBROK "$@ failed"
	fi
}

_tst_expect_pass()
{
	local fnc="$1"
	shift

	tst_rod "$@"
	if [ $? -eq 0 ]; then
		tst_res TPASS "$@ passed as expected"
		return 0
	else
		$fnc TFAIL "$@ failed unexpectedly"
		return 1
	fi
}

_tst_expect_fail()
{
	local fnc="$1"
	shift

	# redirect stderr since we expect the command to fail
	tst_rod "$@" 2> /dev/null
	if [ $? -ne 0 ]; then
		tst_res TPASS "$@ failed as expected"
		return 0
	else
		$fnc TFAIL "$@ passed unexpectedly"
		return 1
	fi
}

EXPECT_PASS()
{
	_tst_expect_pass tst_res "$@"
}

EXPECT_PASS_BRK()
{
	_tst_expect_pass tst_brk "$@"
}

EXPECT_FAIL()
{
	_tst_expect_fail tst_res "$@"
}

EXPECT_FAIL_BRK()
{
	_tst_expect_fail tst_brk "$@"
}
