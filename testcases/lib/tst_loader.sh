#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2024-2025 Cyril Hrubis <chrubis@suse.cz>
#
# This is a loader for shell tests that use the C test library.
#

echo "!!! tst_loader.sh: pid: $$, 1: '$1'" # FIXME: debug
if [ -z "$LTP_IPC_PATH" ]; then
	echo "!!! tst_loader.sh: IF tst_run_shell '$(basename $0)' '$@'" # FIXME: debug
	tst_run_shell $(basename "$0") "$@"
	ret=$?
	echo "!!! tst_loader.sh: pid: $$, explicit exit: $ret" # FIXME: debug
	exit $ret
else
	echo "!!! tst_loader.sh: ELSE pid: $$, . tst_env.sh" # FIXME: debug
	. tst_env.sh
fi

ret=$?
echo "!!! tst_loader.sh: pid: $$, implicit exit at the end: $ret" # FIXME: debug
