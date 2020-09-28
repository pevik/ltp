#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2020 Microsoft Corporation
# Author: Lakshmi Ramasubramanian <nramas@linux.microsoft.com>
#
# Verify measurement of SELinux policy and state

TST_NEEDS_CMDS="awk cut grep tail"
TST_CNT=2
TST_NEEDS_DEVICE=1
TST_SETUP="setup"

. ima_setup.sh

FUNC_CRITICAL_DATA='func=CRITICAL_DATA'
TEMPLATE_BUF='template=ima-buf'
REQUIRED_POLICY="^measure.*($FUNC_CRITICAL_DATA.*$TEMPLATE_BUF|$TEMPLATE_BUF.*$FUNC_CRITICAL_DATA)"

setup()
{
	tst_require_selinux_enabled
	require_ima_policy_content "$REQUIRED_POLICY" '-E' > $TST_TMPDIR/policy.txt
	SELINUX_DIR=$(tst_get_selinux_dir)
}

# Format of the measured SELinux state data.
#
# initialized=1;enabled=1;enforcing=0;checkreqprot=1;
# network_peer_controls=1;open_perms=1;extended_socket_class=1;
# always_check_network=0;cgroup_seclabel=1;nnp_nosuid_transition=1;
# genfs_seclabel_symlinks=0;
validate_policy_capabilities()
{
	local measured_cap measured_value expected_value
	local result=1
	local inx=9

	# Policy capabilities flags start from "network_peer_controls"
	# in the measured SELinux state at offset 9 for 'awk'
	while [ $inx -lt 22 ]; do
		measured_cap=$(echo $1 | awk -F'[=;]' -v inx="$inx" '{print $inx}')
		inx=$(( $inx + 1 ))

		measured_value=$(echo $1 | awk -F'[=;]' -v inx="$inx" '{print $inx}')
		expected_value=$(cat "$SELINUX_DIR/policy_capabilities/$measured_cap")
		if [ "$measured_value" != "$expected_value" ];then
			tst_res TWARN "$measured_cap: expected: $expected_value, got: $digest"
			result=0
		fi

		inx=$(( $inx + 1 ))
	done

	return $result
}

# Trigger measurement of SELinux constructs and verify that
# the measured SELinux policy matches the current policy loaded
# for SELinux.
test1()
{
	local policy_digest expected_policy_digest algorithm
	local data_source_name="selinux"
	local pattern="data_sources=[^[:space:]]*$data_source_name"
	local tmp_file="$TST_TMPDIR/selinux_policy_tmp_file.txt"

	check_policy_pattern "$pattern" $FUNC_CRITICAL_DATA $TEMPLATE_BUF > $tmp_file || return

	tst_res TINFO "verifying SELinux policy measurement"

	# Trigger a measurement by changing SELinux state
	tst_update_selinux_state

	# Verify SELinux policy is measured and then validate that
	# the measured policy matches the policy currently loaded
	# for SELinux
	line=$(grep -E "selinux-policy-hash" $ASCII_MEASUREMENTS | tail -1)
	if [ -z "$line" ]; then
		tst_res TFAIL "SELinux policy not measured"
		return
	fi

	algorithm=$(echo "$line" | cut -d' ' -f4 | cut -d':' -f1)
	policy_digest=$(echo "$line" | cut -d' ' -f6)

	expected_policy_digest="$(compute_digest $algorithm $SELINUX_DIR/policy)" || \
		tst_brk TCONF "cannot compute digest for $algorithm"

	if [ "$policy_digest" != "$expected_policy_digest" ]; then
		tst_res TFAIL "Digest mismatch: expected: $expected_policy_digest, got: $policy_digest"
		return
	fi

	tst_res TPASS "SELinux policy measured correctly"
}

# Trigger measurement of SELinux constructs and verify that
# the measured SELinux state matches the current SELinux
# configuration.
test2()
{
	tst_check_cmds xxd || return

	local measured_data state_file="$TST_TMPDIR/selinux_state.txt"
	local data_source_name="selinux"
	local pattern="data_sources=[^[:space:]]*$data_source_name"
	local tmp_file="$TST_TMPDIR/selinux_state_tmp_file.txt"
	local digest expected_digest algorithm
	local enforced_value expected_enforced_value
	local checkreqprot_value expected_checkreqprot_value
	local result

	tst_res TINFO "verifying SELinux state measurement"

	check_policy_pattern "$pattern" $FUNC_CRITICAL_DATA $TEMPLATE_BUF > $tmp_file || return

	# Trigger a measurement by changing SELinux state
	tst_update_selinux_state

	# Verify SELinux state is measured and then validate the measured
	# state matches that currently set for SELinux
	line=$(grep -E "selinux-state" $ASCII_MEASUREMENTS | tail -1)
	if [ -z "$line" ]; then
		tst_res TFAIL "SELinux state not measured"
		return
	fi

	digest=$(echo "$line" | cut -d' ' -f4 | cut -d':' -f2)
	algorithm=$(echo "$line" | cut -d' ' -f4 | cut -d':' -f1)

	echo "$line" | cut -d' ' -f6 | xxd -r -p > $state_file

	expected_digest="$(compute_digest $algorithm $state_file)" || \
	tst_brk TCONF "cannot compute digest for $algorithm"

	if [ "$digest" != "$expected_digest" ]; then
		tst_res TFAIL "digest mismatch: expected: $expected_digest, got: $digest"
		return
	fi

	measured_data=$(cat $state_file)
	enforced_value=$(echo $measured_data | awk -F'[=;]' '{print $6}')
	expected_enforced_value=$(cat $SELINUX_DIR/enforce)
	if [ "$expected_enforced_value" != "$enforced_value" ];then
		tst_res TFAIL "enforce: expected: $expected_enforced_value, got: $enforced_value"
		return
	fi

	checkreqprot_value=$(echo $measured_data | awk -F'[=;]' '{print $8}')
	expected_checkreqprot_value=$(cat $SELINUX_DIR/checkreqprot)
	if [ "$expected_checkreqprot_value" != "$checkreqprot_value" ];then
		tst_res TFAIL "checkreqprot: expected: $expected_checkreqprot_value, got: $checkreqprot_value"
		return
	fi

	validate_policy_capabilities $measured_data
	result=$?
	if [ $result = 0 ]; then
		tst_res TFAIL "policy capabilities did not match"
		return
	fi

	tst_res TPASS "SELinux state measured correctly"
}

tst_run
