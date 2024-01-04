#!/bin/sh

fail=0

for i in *.c; do
	../metaparse $i > tmp.json
	[ "$VERBOSE" ] && echo "***** Parsing $i *****"

	if ! diff tmp.json $i.json >/dev/null 2>&1; then
		echo "***"
		echo "$i output differs!"
		diff -u tmp.json $i.json
		echo "***"
		fail=1
		[ "$VERBOSE" ] && echo "$i: FAIL"
	else
		[ "$VERBOSE" ] && echo "$i: PASS"
	fi
done

rm -f tmp.json

exit $fail
