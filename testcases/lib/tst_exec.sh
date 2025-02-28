#!/bin/sh
# Copyright (c) 2025 Cyril Hrubis <chrubis@suse.cz>
# Copyright (c) 2025 Petr Vorel <pvorel@suse.cz>

. tst_env.sh

. "$1"

if [ -n "$TST_CLEANUP" ]; then
	trap $TST_CLEANUP EXIT
fi

if [ -n "$TST_SETUP" ]; then
    $TST_SETUP
fi

tst_test

# vim: set ft=sh ts=4 sts=4 sw=4 expandtab :
