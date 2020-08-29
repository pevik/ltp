#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2020 Microsoft Corporation
# Author: Tushar Sugandhi <tusharsu@linux.microsoft.com>
#
# Verify that DM target dm-crypt is measured correctly based on policy.

TST_NEEDS_CMDS="grep cut sed tr dmsetup"
TST_CNT=1
TST_NEEDS_DEVICE=1
TST_SETUP=setup

. ima_setup.sh

FUNC_CRIT_DATA='func=CRITICAL_DATA'
TEMPLATE_BUF='template=ima-buf'
REQUIRED_POLICY="^measure.*($FUNC_CRIT_DATA.*$TEMPLATE_BUF|$TEMPLATE_BUF.*$FUNC_CRIT_DATA)"

setup()
{
	tst_res TINFO "inside setup"
	require_ima_policy_content "$REQUIRED_POLICY" '-E' > policy.txt
}

check_dm_crypt_policy()
{
	local pattern="$1"

	if ! grep -E "$pattern" policy.txt; then
		tst_res TCONF "IMA policy must specify $pattern, $FUNC_CRIT_DATA, $TEMPLATE_BUF"
		return 1
	fi
	return 0
}

test1()
{
	local dmcheck_lines i dm_targets templates
	local policy="critical_kernel_data_sources"
	local pattern='critical_kernel_data_sources=[^[:space:]]+'
	local tmp_file="tmp.txt"
	local tokens_file="tokens_file.txt" grep_file="grep_file.txt"
	local arg cmd key tgt_name
	local res=0

	tst_res TINFO "verifying dm target - dmcrypt gets measured correctly."

	check_dm_crypt_policy "$pattern" > $tmp_file || return

	dmcheck_lines=$(cat $tmp_file)
	dm_targets=$(for i in $dmcheck_lines; do echo "$i" | grep "$policy" | \
		sed "s/\./\\\./g" | cut -d'=' -f2; done | sed ':a;N;$!ba;s/\n/|/g')
	if [ -z "$dm_targets" ]; then
		tst_res TCONF "IMA policy has a $policy key-value specifier, but no specified sources."
		return
	fi

	templates=$(for i in $dmcheck_lines; do echo "$i" | grep "template" | \
		cut -d'=' -f2; done | sed ':a;N;$!ba;s/\n/|/g')

	tst_res TINFO "dm_targets: '$dm_targets'"
	tst_res TINFO "templates: '$templates'"

	tgt="crypt"
	key="faf453b4ee938cff2f0d2c869a0b743f59125c0a37f5bcd8f1dbbd911a78abaa"

	arg="'0 1953125 crypt aes-xts-plain64 "
	arg="$arg $key 0 "
	arg="$arg /dev/loop0 0 1 allow_discards'"
	tgt_name="test-crypt"
	cmd="dmsetup create $tgt_name --table $arg"

	tst_res TINFO "Executing: $cmd"
	eval $cmd

	grep -E "($templates)*($dm_targets)" $ASCII_MEASUREMENTS > $grep_file

	while read line
	do
		local act_digest exp_digest comp_digest algorithm

		act_digest=$(echo "$line" | cut -d' ' -f4 | cut -d':' -f2)
		algorithm=$(echo "$line" | cut -d' ' -f4 | cut -d':' -f1)
		dmtgt_evtname=$(echo "$line" | cut -d' ' -f5)

		echo "$line" | cut -d' ' -f6 | xxd -r -p > $tokens_file
		plain_text=$(echo "$line" | cut -d' ' -f6 | xxd -r -p)

		#expected digest for $cmd
		exp_digest="039d8ff71918608d585adca3e5aab2e3f41f84d6"
		comp_digest="$(compute_digest $algorithm $tokens_file)" || \
			tst_brk TCONF "cannot compute digest for $algorithm"

		if [ "$act_digest" != "$comp_digest" ]; then
			tst_res TFAIL "Incorrect digest for ($dmtgt_evtname)."
			tst_res TFAIL "Expected digest:($comp_digest)."
			tst_res TFAIL "Actual digest:($act_digest)."
			tst_res TINFO "Removing DM target $tgt_name."
			dmsetup remove $tgt_name
			return
		fi

		if [ "$act_digest" = "$exp_digest" ]; then
			res=1
		fi

	done < $grep_file

	if [ $res -eq 1 ]; then
		tst_res TPASS "dm-crypt target verification passed."
	else
		tst_res TFAIL "dm-crypt target verification failed."
	fi
	tst_res TINFO "Removing DM target $tgt_name."
	dmsetup remove $tgt_name
}

tst_run
