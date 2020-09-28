#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2020 Microsoft Corporation
# Author: Tushar Sugandhi <tusharsu@linux.microsoft.com>
#
# Verify that DM target dm-crypt are measured correctly based on policy.

TST_NEEDS_CMDS="dmsetup"
TST_CNT=1
TST_NEEDS_DEVICE=1
TST_SETUP=setup
TST_CLEANUP=cleanup

. ima_setup.sh

FUNC_CRIT_DATA='func=CRITICAL_DATA'
TEMPLATE_BUF='template=ima-buf'
REQUIRED_POLICY="^measure.*($FUNC_CRIT_DATA.*$TEMPLATE_BUF|$TEMPLATE_BUF.*$FUNC_CRIT_DATA)"

setup()
{
	require_ima_policy_content "$REQUIRED_POLICY" '-E' > $TST_TMPDIR/policy.txt
}

cleanup()
{
	dmsetup remove test-crypt
}

test1()
{
	local input_digest="039d8ff71918608d585adca3e5aab2e3f41f84d6"
	local pattern='data_sources=[^[:space:]]+'
	local tmp_file="$TST_TMPDIR/dm_crypt_tmp.txt"
	local policy="data_sources"
	local arg key

	tst_res TINFO "verifying dm target - dmcrypt gets measured correctly"

	check_policy_pattern "$pattern" $FUNC_CRIT_DATA $TEMPLATE_BUF > $tmp_file || return

	tgt="crypt"
	key="faf453b4ee938cff2f0d2c869a0b743f59125c0a37f5bcd8f1dbbd911a78abaa"

	arg="'0 1953125 crypt aes-xts-plain64 "
	arg="$arg $key 0 "
	arg="$arg /dev/loop0 0 1 allow_discards'"

	ROD dmsetup create test-crypt --table $arg

	if check_ima_ascii_log_for_policy $policy $tmp_file $input_digest; then
		tst_res TPASS "dm-crypt target verification passed"
	else
		tst_res TFAIL "dm-crypt target verification failed"
	fi
}

tst_run
