#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2024-2025 Cyril Hrubis <chrubis@suse.cz>
#
# This is a loader for shell tests that use the C test library.

if [ -z "$LTP_IPC_PATH" ]; then
	tst_run_shell $(basename "$0") "$@"
	exit $?
fi

. tst_env.sh

if [ -n "$TST_CLEANUP" ]; then
	trap $TST_CLEANUP EXIT
fi

if [ -n "$TST_SETUP" ]; then
    $TST_SETUP
fi

tst_test
