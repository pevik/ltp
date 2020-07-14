#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2020 Microsoft Corporation
# Author: Lachlan Sneff <t-josne@linux.microsoft.com>
#
# Verify that keys are measured correctly based on policy.

TST_NEEDS_CMDS="grep mktemp cut sed tr xxd keyctl evmctl openssl cmp"
TST_CNT=2
TST_NEEDS_DEVICE=1

CERT_FILE="${CERT_FILE:-/etc/keys/x509_ima.der}"

. ima_setup.sh

# Based on https://lkml.org/lkml/2019/12/13/564.
# (450d0fd51564 - "IMA: Call workqueue functions to measure queued keys")
test1()
{
	local keyrings keycheck_lines keycheck_line templates test_file="file.txt" fail

	tst_res TINFO "verifying key measurement for keyrings and templates specified in IMA policy file"

	[ -f $IMA_POLICY ] || tst_brk TCONF "missing $IMA_POLICY"

	[ -r $IMA_POLICY ] || tst_brk TCONF "cannot read IMA policy (CONFIG_IMA_READ_POLICY=y required)"

	keycheck_lines=$(grep "func=KEY_CHECK" $IMA_POLICY)
	if [ -z "$keycheck_lines" ]; then
		tst_brk TCONF "ima policy does not specify \"func=KEY_CHECK\""
	fi

	keycheck_line=$(echo "$keycheck_lines" | grep "keyrings" | head -n1)

	if [ -z "$keycheck_line" ]; then
		tst_brk TCONF "ima policy does not specify a keyrings to check"
	fi

	keyrings=$(echo "$keycheck_line" | tr " " "\n" | grep "keyrings" | \
		sed "s/\./\\\./g" | cut -d'=' -f2)
	if [ -z "$keyrings" ]; then
		tst_brk TCONF "ima policy has a keyring key-value specifier, but no specified keyrings"
	fi

	templates=$(echo "$keycheck_line" | tr " " "\n" | grep "template" | \
		cut -d'=' -f2)

	grep -E "($templates)*($keyrings)" $ASCII_MEASUREMENTS | while read line
	do
		local digest expected_digest algorithm

		digest=$(echo "$line" | cut -d' ' -f4 | cut -d':' -f2)
		algorithm=$(echo "$line" | cut -d' ' -f4 | cut -d':' -f1)
		keyring=$(echo "$line" | cut -d' ' -f5)

		echo "$line" | cut -d' ' -f6 | xxd -r -p > $test_file

		expected_digest="$(compute_digest $algorithm $test_file)" || \
			tst_brk TCONF "cannot compute digest for $algorithm"

		if [ "$digest" != "$expected_digest" ]; then
			fail=1
			tst_res TFAIL "incorrect digest was found for the ($keyring) keyring"
			break
		fi
	done

	if [ "$fail" ]; then
		tst_res TPASS "specified keyrings were measured correctly"
	fi
}


# Test that a cert can be imported into the ".ima" keyring correctly.
test2() {
	local keyring_id key_id test_file="file.txt"

	[ -f $CERT_FILE ] || tst_brk TCONF "missing $CERT_FILE"

	if ! openssl x509 -in $CERT_FILE -inform der > /dev/null; then
		tst_brk TCONF "The suppled cert file ($CERT_FILE) is not a valid x509 certificate"
	fi

	tst_res TINFO "adding a cert to the .ima keyring ($CERT_FILE)"

	keyring_id=$(keyctl describe %:.ima | cut -d' ' -f2 | tr -d ':') || \
		tst_btk TCONF "unable to retrieve .ima keyring id"

	if ! tst_is_num	"$keyring_id"; then
		tst_brk TCONF "unable to parse keyring id from keyring"
	fi

	evmctl import $CERT_FILE "$keyring_id" > /dev/null || \
		tst_brk TCONF "unable to import a cert into the .ima keyring"

	grep -F ".ima" "$ASCII_MEASUREMENTS" | tail -n1 | cut -d' ' -f6 | \
		xxd -r -p > $test_file || \
		tst_brk TCONF "cert not found in ascii_runtime_measurements log"

	if ! openssl x509 -in $test_file -inform der > /dev/null; then
		tst_brk TCONF "The cert logged in ascii_runtime_measurements is not a valid x509 certificate"
	fi

	if cmp -s "$test_file" $CERT_FILE; then
		tst_res TPASS "logged cert matches original cert"
	else
		tst_res TFAIL "logged cert does not match original cert"
	fi
}

tst_run
