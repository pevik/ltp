#!/bin/sh -eu
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2021 SUSE LLC  <rpalethorpe@suse.com>

# Run the Coccinelle checks for the library. Running the fixes
# requires passing -D fix instead of -D report.

if [[ ! -d lib || ! -d scripts/coccinelle ]]; then
    echo "$0: Can't find lib or scripts directories. Run me from top src dir"
    exit 1
fi

echo Python args ${COCCI_PYTHON:=--python python3} >&2

libltp_spatch() {
    spatch $COCCI_PYTHON --dir lib \
	   --ignore lib/parse_opts.c \
	   --ignore lib/newlib_tests \
	   --ignore lib/tests \
	   --very-quiet \
	   --use-gitgrep \
	   -D report \
	   --include-headers \
	   $*
}

libltp_spatch --sp-file scripts/coccinelle/libltp-test-macro.cocci
libltp_spatch --sp-file scripts/coccinelle/libltp-test-macro-vars.cocci \
	      --ignore lib/tst_test.c
