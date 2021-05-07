#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2021 Microsoft Corporation
# Copyright (c) 2021-2023 Petr Vorel <pvorel@suse.cz>
# Author: Tushar Sugandhi <tusharsu@linux.microsoft.com>
#
# Verify that DM target dm-crypt are measured correctly based on policy.

TST_NEEDS_CMDS="dmsetup"
TST_NEEDS_DEVICE=1
TST_SETUP=setup
TST_CLEANUP=cleanup

POLICY_FUNC='func=CRITICAL_DATA'
PATTERN='label=device-mapper'
REQUIRED_POLICY="^measure.*($FUNC.*$PATTERN|$PATTERN.*$FUNC)"
POLICY_FILE="$TST_TMPDIR/policy.txt"

setup()
{
	require_ima_policy_content "$REQUIRED_POLICY" '-E' > $POLICY_FILE
	require_valid_policy_template $POLICY_FUNC $POLICY_FILE
}

cleanup()
{
	[ "$dmsetup_run" ] || return
	dmsetup remove test-crypt
}

test1()
{
	local input_digest="039d8ff71918608d585adca3e5aab2e3f41f84d6"
	local key="faf453b4ee938cff2f0d2c869a0b743f59125c0a37f5bcd8f1dbbd911a78abaa"

	tst_res TINFO "verifying dm-crypt target measurement"

	ROD dmsetup create test-crypt --table "0 1953125 crypt aes-xts-plain64 $key 0 /dev/loop0 0 1 allow_discards"
	# FIXME: missing function
	# https://patchwork.ozlabs.org/project/ltp/patch/20200928035605.22701-3-tusharsu@linux.microsoft.com/
	# contains check_ima_ascii_log_for_policy which does not exist
	check_policy_measurement $policy $POLICY_FILE $input_digest
}

. ima_setup.sh
tst_run
