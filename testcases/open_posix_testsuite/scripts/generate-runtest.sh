#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2026 Linaro Limited

if [ $# -ne 3 ]; then
	echo "Usage: $(basename "$0") <installed testdir> <testdir relative to LTPROOT> <suite>" >&2
	exit 1
fi

installed_testdir=$1
testdir_rel=$2
suite=$3

cd "$installed_testdir" || exit 1

if [ ! -d "$suite" ]; then
	echo "$installed_testdir/$suite does not exist" >&2
	exit 1
fi

run_tests_line='^[^ ]*/run-tests\.sh [^ ]* '

find "$suite" -name speculative -prune -o -name run.sh -print | sort | \
while read -r run_sh; do
	if ! grep -q "$run_tests_line" "$run_sh"; then
		echo "$run_sh has no run-tests.sh line" >&2
		exit 1
	fi
	dir=${run_sh%/run.sh}
	tests=$(sed -n "s,$run_tests_line,,p" "$run_sh")
	for test_name in $tests; do
		[ -f "$dir/$test_name" ] || continue
		base=${test_name%.*}
		args=
		[ -f "$dir/$base.args" ] && read -r args < "$dir/$base.args"
		echo "$base openposix_run.sh ${testdir_rel:+$testdir_rel/}$dir/$test_name${args:+ $args}"
	done
done
