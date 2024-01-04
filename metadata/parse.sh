#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2019 Cyril Hrubis <chrubis@suse.cz>
# Copyright (c) 2020 Petr Vorel <pvorel@suse.cz>
# Copyright (c) Linux Test Project, 2022-2024
set -e

top_builddir=$PWD/..
top_srcdir="$(cd $(dirname $0)/..; pwd)"

cd $top_srcdir

version=$(cat $top_srcdir/VERSION)
if [ -d .git ]; then
	version=$(git describe 2>/dev/null) || version=$(cat $top_srcdir/VERSION).GIT-UNKNOWN
fi

[ "$V" ] && VERBOSE=1

echo '{'
echo ' "testsuite": {'
echo '  "name": "Linux Test Project",'
echo '  "short_name": "LTP",'
echo '  "url": "https://github.com/linux-test-project/ltp/",'
echo '  "scm_url_base": "https://github.com/linux-test-project/ltp/tree/master/",'
echo "  \"version\": \"$version\""
echo ' },'
echo ' "defaults": {'
echo '  "timeout": 30'
echo ' },'
echo ' "tests": {'

parse()
{
	local test="$1"

	a=$($top_builddir/metadata/metaparse $VERBOSE -Iinclude -Itestcases/kernel/syscalls/utils/ "$test")
	if [ -n "$a" ]; then
		if [ -z "$first" ]; then
			echo ','
		fi
		first=
		cat <<EOF
$a
EOF
	fi
}

first=1

if [ $# -gt 0 ]; then
	for test in "$@"; do
		parse "$test"
	done
else
	for test in $(find testcases/ -name '*.c' | sort); do
		parse "$test"
	done
fi

echo
echo ' }'
echo '}'
