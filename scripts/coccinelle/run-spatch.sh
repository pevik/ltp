#!/bin/sh -eu
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2021 SUSE LLC  <rpalethorpe@suse.com>

# Helper for running spatch Coccinelle scripts on the LTP source tree

if [[ ! -d lib || ! -d scripts/coccinelle ]]; then
    echo "$0: Can't find lib or scripts directories. Run me from top src dir"
    exit 1
fi

# Run a script on the lib dir
libltp_spatch_report() {
    spatch --dir lib \
	   --ignore lib/parse_opts.c \
	   --ignore lib/newlib_tests \
	   --ignore lib/tests \
	   --use-gitgrep \
	   -D report \
	   --include-headers \
	   $*
}

libltp_spatch_fix() {
    spatch --dir lib \
	   --ignore lib/parse_opts.c \
	   --ignore lib/newlib_tests \
	   --ignore lib/tests \
	   --use-gitgrep \
	   --in-place \
	   -D fix \
	   --include-headers \
	   $*
}

echo You should uncomment one of the scripts below!
#libltp_spatch_report --sp-file scripts/coccinelle/libltp-test-macro.cocci
#libltp_spatch_report --sp-file scripts/coccinelle/libltp-test-macro-vars.cocci \
#		     --ignore lib/tst_test.c
