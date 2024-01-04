#!/bin/sh

fail=0

for i in *.c; do
	[ "$VERBOSE" ] && echo "$0: testing $i"
	../metaparse $i > tmp.json
	if ! diff tmp.json $i.json >/dev/null 2>&1; then
		echo "***"
		echo "$i output differs!"
		diff -u tmp.json $i.json
		echo "***"
		fail=1
	fi
done

rm -f tmp.json

[ "$VERBOSE" ] && echo "$fail"
exit $fail
