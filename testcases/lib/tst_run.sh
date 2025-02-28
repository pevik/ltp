#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2025 Cyril Hrubis <chrubis@suse.cz>
# Copyright (c) 2025 Petr Vorel <pvorel@suse.cz>

. tst_env.sh

if [ -n "$TST_CLEANUP" ]; then
	trap $TST_CLEANUP EXIT
fi

if [ -n "$TST_SETUP" ]; then
    $TST_SETUP
fi

tst_test
