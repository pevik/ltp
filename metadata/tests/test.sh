#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) Linux Test Project, 2021-2026

fail=0

BUILDDIR="${BUILDDIR:-.}"

cd "${SRCDIR:-.}"

for i in *.c; do
	printf "* $i "
	$BUILDDIR/../metaparse $i > $BUILDDIR/tmp.json
	if ! diff $BUILDDIR/tmp.json $i.json >/dev/null 2>&1; then
		echo '[FAIL]'
		echo "$i output differs!"
		diff -u $BUILDDIR/tmp.json $i.json
		fail=1
	else
		echo '[OK]'
	fi
done

rm -f $BUILDDIR/tmp.json

exit $fail
