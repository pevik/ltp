#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2009 IBM Corporation
# Copyright (c) 2018-2020 Petr Vorel <pvorel@suse.cz>
# Author: Mimi Zohar <zohar@linux.ibm.com>
#
# Verify the boot and PCR aggregates.

TST_CNT=2
TST_NEEDS_CMDS="awk cut"
TST_SETUP="setup"

. ima_setup.sh

EVMCTL_REQUIRED='1.3'
ERRMSG_EVMCTL="install evmctl >= $EVMCTL_REQUIRED"
ERRMSG_TPM="TPM hardware support not enabled in kernel or no TPM chip found"

setup()
{
	local line tmp

	read line < $ASCII_MEASUREMENTS
	if tmp=$(get_algorithm_digest "$line"); then
		ALGORITHM=$(echo "$tmp" | cut -d'|' -f1)
		DIGEST=$(echo "$tmp" | cut -d'|' -f2)
	else
		tst_res TBROK "failed to get algorithm/digest: $tmp"
	fi
	tst_res TINFO "used algorithm: $ALGORITHM"

	if ! check_evmctl $EVMCTL_REQUIRED; then
		MISSING_EVMCTL=1
		if [ "$ALGORITHM" = "sha1" ]; then
			tst_brk TCONF "algorithm not sha1 ($ALGORITHM), $ERRMSG_EVMCTL"
		fi
	fi
}

# check_evmctl REQUIRED_TPM_VERSION
# return: 0: evmctl is new enough, 1: too old version
check_evmctl()
{
	local required="$1"

	local r1="$(echo $required | cut -d. -f1)"
	local r2="$(echo $required | cut -d. -f2)"
	local r3="$(echo $required | cut -d. -f3)"
	[ -z "$r3" ] && r3=0

	tst_is_int "$r1" || tst_brk TBROK "required major version not int ($v1)"
	tst_is_int "$r2" || tst_brk TBROK "required minor version not int ($v2)"
	tst_is_int "$r3" || tst_brk TBROK "required patch version not int ($v3)"

	tst_check_cmds evmctl || return 1

	local v="$(evmctl --version | cut -d' ' -f2)"
	tst_res TINFO "evmctl version: $v"

	local v1="$(echo $v | cut -d. -f1)"
	local v2="$(echo $v | cut -d. -f2)"
	local v3="$(echo $v | cut -d. -f3)"
	[ -z "$v3" ] && v3=0

	if [ $v1 -lt $r1 -o $v2 -lt $r2 -o $v3 -lt $r3 ]; then
		tst_res TCONF "evmctl >= $required required ($v)"
		return 1
	fi
	return 0
}

# prints major version 1: TPM 1.2, 2: TPM 2.0
# or nothing when version not detected
get_tpm_version()
{
	if [ -f /sys/class/tpm/tpm0/tpm_version_major ]; then
		cat /sys/class/tpm/tpm0/tpm_version_major
		return
	fi

	if [ -f /sys/class/tpm/tpm0/device/caps -o \
		-f /sys/class/misc/tpm0/device/caps ]; then
		echo 1
		return
	fi

	tst_check_cmds dmesg || return
	if dmesg | grep -q '1\.2 TPM (device-id'; then
		echo 1
	elif dmesg | grep -q '2\.0 TPM (device-id'; then
		echo 2
	fi
}

read_pcr_tpm1()
{
	local pcr_path="/sys/class/tpm/tpm0/device/pcrs"
	local pcr hash

	if [ ! -f "$pcrs_path" ]; then
		pcrs_path="/sys/class/misc/tpm0/device/pcrs"
	fi

	if [ ! -f "$pcr_path" ]; then
		tst_brk TCONF "missing PCR file $pcrs_path ($ERRMSG_TPM)"
	fi

	while read line; do
		pcr="$(echo $line | cut -d':' -f1)"
		hash="$(echo $line | cut -d':' -f2 | awk '{ gsub (" ", "", $0); print tolower($0) }')"
		echo "$pcr: $hash"
	done < $pcr_path
}

# NOTE: TPM 1.2 would require to use tss1pcrread which is not fully adopted
# by distros yet.
read_pcr_tpm2()
{
	local pcrmax=23
	local pcrread="tsspcrread -halg $ALGORITHM"
	local i pcr

	tst_check_cmds tsspcrread || return 1

	for i in $(seq 0 $pcrmax); do
		pcr=$($pcrread -ha "$i" -ns)
		if [ $? -ne 0 ]; then
			tst_brk TBROK "tsspcrread failed: $pcr"
		fi
		printf "PCR-%02d: %s\n" $i "$pcr"
	done
}

get_pcr10_aggregate()
{
	local pcr

	if ! evmctl -v ima_measurement $BINARY_MEASUREMENTS > hash.txt 2>&1; then
		tst_res TBROK "evmctl failed:"
		cat hash.txt >&2
		return 1
	fi
	pcr=$(grep -E "^($ALGORITHM: TPM |HW )*PCR-10:" hash.txt \
		| awk '{print $NF}')

	echo "$pcr"
}

test1()
{
	tst_res TINFO "verify boot aggregate"

	local tpm_bios="$SECURITYFS/tpm0/binary_bios_measurements"
	local cmd="evmctl ima_boot_aggregate"
	local boot_aggregate cmd zero

	if [ "$MISSING_EVMCTL" = 1 ]; then
		if [ -f "$tpm_bios" ]; then
			tst_res TCONF "missing $tpm_bios, $ERRMSG_EVMCTL"
			return
		fi
		cmd="ima_boot_aggregate"
	fi
	tst_res TINFO "using command: $cmd"

	boot_aggregate=$($cmd | grep "$ALGORITHM:" | cut -d':' -f2)
	if [ -z "$boot_aggregate" ]; then
		tst_res TINFO "$ERRMSG_TPM"

		zero=$(echo $DIGEST | awk '{gsub(/./, "0")}; {print}')
		if [ "$DIGEST" = "$zero" ]; then
			tst_res TPASS "bios boot aggregate is $zero"
		else
			tst_res TFAIL "bios boot aggregate is not $zero ($DIGEST)"
		fi
		return
	fi

	tst_res TINFO "IMA boot aggregate: '$boot_aggregate'"

	if [ "$DIGEST" = "$boot_aggregate" ]; then
		tst_res TPASS "bios boot aggregate matches IMA boot aggregate"
	else
		tst_res TFAIL "bios boot aggregate does not match IMA boot aggregate ($DIGEST)"
	fi
}

test2()
{
	local hash pcr_aggregate

	tst_res TINFO "verify PCR values"

	tpm_version="$(get_tpm_version)"
	if [ -z "$tpm_version" ]; then
		tst_brk TCONF "TMP version not detected ($ERRMSG_TPM)"
	fi
	tst_res TINFO "TMP major version: $tpm_version"

	read_pcr_tpm$tpm_version > pcr.txt
	hash=$(grep "^PCR-10" pcr.txt | cut -d' ' -f2)
	if [ -z "$hash" ]; then
		tst_res TBROK "PCR-10 hash not found"
		cat pcr.txt
		return 1
	fi
	tst_res TINFO "real PCR-10: '$hash'"

	pcr_aggregate="$(get_pcr10_aggregate)"
	if [ -z "$pcr_aggregate" ]; then
		tst_res TFAIL "failed to get aggregate PCR-10"
		return
	fi
	tst_res TINFO "aggregate PCR-10: '$hash'"

	if [ "$hash" = "$pcr_aggregate" ]; then
		tst_res TPASS "aggregate PCR value matches real PCR value"
	else
		tst_res TFAIL "aggregate PCR value does not match real PCR value"
	fi
}

tst_run
