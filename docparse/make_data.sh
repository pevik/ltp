#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later

if [ $# -ne 1 ]; then
	echo "usage $0 FILE" >&2
	exit 1
fi

echo "var metadata ="
cat $1
