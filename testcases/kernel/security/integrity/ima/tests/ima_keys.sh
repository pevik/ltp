#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2020 Microsoft Corporation
# Copyright (c) 2020-2021 Petr Vorel <pvorel@suse.cz>
# Author: Lachlan Sneff <t-josne@linux.microsoft.com>
#
# Verify that keys are measured correctly based on policy.

TST_NEEDS_CMDS="cmp cut grep xxd"
TST_CNT=2
TST_NEEDS_DEVICE=1
TST_SETUP=setup
TST_CLEANUP=cleanup

. ima_setup.sh

POLICY_FUNC='func=KEY_CHECK'
REQUIRED_POLICY="^measure.*$POLICY_FUNC"
POLICY_FILE="$TST_TMPDIR/policy.txt"

setup()
{
	require_ima_policy_content "$REQUIRED_POLICY" '-E' > $POLICY_FILE
	require_valid_policy_template $FUNC $POLICY_FILE
}

cleanup()
{
	tst_is_num $KEYRING_ID && keyctl clear $KEYRING_ID
}

# Based on https://lkml.org/lkml/2019/12/13/564.
# (450d0fd51564 - "IMA: Call workqueue functions to measure queued keys")
test1()
{
	local keycheck_lines i keyrings templates
	local pattern='keyrings=[^[:space:]]+'
	local policy="keyrings"
	local tmp_file="$TST_TMPDIR/keycheck_tmp_file.txt"

	tst_res TINFO "verify key measurement for keyrings and templates specified in IMA policy"

	check_policy_pattern "$pattern" $POLICY_FUNC $POLICY_FILE > $tmp_file || return
	test_policy_measurement $policy $temp_file
}

# Create a new keyring, import a certificate into it, and verify
# that the certificate is measured correctly by IMA.
test2()
{
	tst_require_cmds keyctl openssl

	require_evmctl "1.3.2"

	local cert_file="$TST_DATAROOT/x509_ima.der"
	local keyring_name="key_import_test"
	local pattern="keyrings=[^[:space:]]*$keyring_name"
	local temp_file="$TST_TMPDIR/key_import_test_file.txt"

	tst_res TINFO "verify measurement of certificate imported into a keyring"

	check_policy_pattern "$pattern" $POLICY_FUNC $POLICY_FILE >/dev/null || return

	KEYRING_ID=$(keyctl newring $keyring_name @s) || \
		tst_brk TBROK "unable to create a new keyring"

	if ! tst_is_num $KEYRING_ID; then
		tst_brk TBROK "unable to parse the new keyring id ('$KEYRING_ID')"
	fi

	evmctl import $cert_file $KEYRING_ID > /dev/null || \
		tst_brk TBROK "unable to import a certificate into $keyring_name keyring"

	grep $keyring_name $ASCII_MEASUREMENTS | tail -n1 | cut -d' ' -f6 | \
		xxd -r -p > $temp_file

	if [ ! -s $temp_file ]; then
		tst_res TFAIL "keyring $keyring_name not found in $ASCII_MEASUREMENTS"
		return
	fi

	if ! openssl x509 -in $temp_file -inform der > /dev/null; then
		tst_res TFAIL "logged certificate is not a valid x509 certificate"
		return
	fi

	if cmp -s $temp_file $cert_file; then
		tst_res TPASS "logged certificate matches the original"
	else
		tst_res TFAIL "logged certificate does not match original"
	fi
}

tst_run
