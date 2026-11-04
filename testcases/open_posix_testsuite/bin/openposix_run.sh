#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2026 Linaro Limited

LTPROOT=${LTPROOT:-$(cd "$(dirname "$0")/../.." && pwd)}

if [ $# -lt 1 ]; then
	echo "Usage: $(basename "$0") <test path relative to LTPROOT> [args]" >&2
	exit 2
fi

test_path=$LTPROOT/$1
test_file=${test_path##*/}
shift

if [ ! -x "$test_path" ]; then
	echo "$test_path is missing or not executable" >&2
	exit 2
fi

cd "${test_path%/*}" || exit 2

./"$test_file" "$@"
ret=$?

case $ret in
4|5) exit 32 ;;
*) exit $ret ;;
esac
