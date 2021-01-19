#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2020 Microsoft Corporation
# Copyright (c) 2020 Petr Vorel <pvorel@suse.cz>
# Author: Lachlan Sneff <t-josne@linux.microsoft.com>
#
# Verify that keys are measured correctly based on policy.

TST_NEEDS_CMDS="cmp cut grep xxd"
TST_CNT=2
TST_NEEDS_DEVICE=1
TST_SETUP=setup
TST_CLEANUP=cleanup

. ima_setup.sh

FUNC_KEYCHECK='func=KEY_CHECK'
TEMPLATE_BUF='template=ima-buf'
REQUIRED_POLICY="^measure.*($FUNC_KEYCHECK.*$TEMPLATE_BUF|$TEMPLATE_BUF.*$FUNC_KEYCHECK)"

setup()
{
	require_ima_policy_content "$REQUIRED_POLICY" '-E' > $TST_TMPDIR/policy.txt
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
	local res

	tst_res TINFO "verify key measurement for keyrings and templates specified in IMA policy"

	check_policy_pattern "$pattern" $FUNC_KEYCHECK $TEMPLATE_BUF > $tmp_file || return

	res="$(check_ima_ascii_log_for_policy $policy $tmp_file)"

	if [ "$res" = "0" ]; then
		tst_res TPASS "specified keyrings were measured correctly"
	else
		tst_res TFAIL "failed to measure specified keyrings"
	fi

}

# Create a new keyring, import a certificate into it, and verify
# that the certificate is measured correctly by IMA.
test2()
{
	tst_require_cmds evmctl keyctl openssl

	local cert_file="$TST_DATAROOT/x509_ima.der"
	local keyring_name="key_import_test"
	local pattern="keyrings=[^[:space:]]*$keyring_name"
	local temp_file="$TST_TMPDIR/key_import_test_file.txt"

	tst_res TINFO "verify measurement of certificate imported into a keyring"

	check_policy_pattern "$pattern" $FUNC_KEYCHECK $TEMPLATE_BUF >/dev/null || return

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
