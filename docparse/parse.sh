#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later

cd ..

version=$(git describe)

echo '{'
echo ' "testsuite": "Linux Test Project",'
echo ' "testsuite_short": "LTP",'
echo ' "url": "https://github.com/linux-test-project/ltp/",'
echo ' "scm_url_base": "https://github.com/linux-test-project/ltp/tree/master/",'
echo ' "timeout": 300,'
echo " \"version\": \"$version\","
echo ' "tests": {'

first=1

for test in `find testcases/ -name '*.c'`; do
	a=$(./docparse/docparse "$test")
	if [ -n "$a" ]; then
		if [ -z "$first" ]; then
			echo ','
		fi
		first=
		echo -n "$a"
	fi
done

echo
echo ' }'
echo '}'
