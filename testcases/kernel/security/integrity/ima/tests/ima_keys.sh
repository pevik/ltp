#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2020 Microsoft Corporation
# Author: Lachlan Sneff <t-josne@linux.microsoft.com>
#
# Verify that keys are measured correctly based on policy.

TST_NEEDS_CMDS="awk cut"
TST_SETUP="setup"
TST_CNT=1
TST_NEEDS_DEVICE=1

. ima_setup.sh
. compute_digest.sh

setup()
{
    TEST_FILE="$PWD/test.txt"
}

# Based on https://lkml.org/lkml/2019/12/13/564.
test1()
{
	local keyrings keycheck_line templates

	tst_res TINFO "verifying key measurement for keyrings and \
templates specified in ima policy file"

	IMA_POLICY="$IMA_DIR/policy"
	[ -f $IMA_POLICY ] || tst_brk TCONF "missing $IMA_POLICY"

	keycheck_line=$(grep "func=KEY_CHECK" $IMA_POLICY)
	if [ -z "$keycheck_line" ]; then
		tst_brk TCONF "ima policy does not specify \"func=KEY_CHECK\""
	fi

	if echo "$keycheck_line" | grep -q "*keyrings*"; then
		tst_brk TCONF "ima policy does not specify a keyrings to check"
	fi

	keyrings=$(echo "$keycheck_line" | tr " " "\n" | grep "keyrings" | \
		sed "s/\./\\\./g" | cut -d'=' -f2)
	if [ -z "$keyrings" ]; then
		tst_brk TCONF "ima policy has a keyring key-value specifier, \
but no specified keyrings"
	fi

	templates=$(echo "$keycheck_line" | tr " " "\n" | grep "template" | \
		cut -d'=' -f2)

	grep -E "($templates)*($keyrings)" $ASCII_MEASUREMENTS | while read line
	do
		local digest expected_digest algorithm

		digest=$(echo "$line" | cut -d' ' -f4 | cut -d':' -f2)
		algorithm=$(echo "$line" | cut -d' ' -f4 | cut -d':' -f1)

		echo "$line" | cut -d' ' -f6 | xxd -r -p > $TEST_FILE

		expected_digest="$(compute_digest $algorithm $TEST_FILE)" || \
			tst_brk TCONF "cannot compute digest for $algorithm"

		if [ "$digest" != "$expected_digest" ]; then
			tst_res TFAIL "incorrect digest was found for the \
$(echo "$line" | cut -d' ' -f5) keyring"
		fi
	done

	tst_res TPASS "specified keyrings were measured correctly"
}

tst_run
