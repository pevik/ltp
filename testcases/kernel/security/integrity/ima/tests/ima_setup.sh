#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2009 IBM Corporation
# Copyright (c) 2018-2019 Petr Vorel <pvorel@suse.cz>
# Author: Mimi Zohar <zohar@linux.ibm.com>

TST_TESTFUNC="test"
TST_SETUP_CALLER="$TST_SETUP"
TST_SETUP="ima_setup"
TST_CLEANUP_CALLER="$TST_CLEANUP"
TST_CLEANUP="ima_cleanup"
TST_NEEDS_ROOT=1

# TST_NEEDS_DEVICE can be unset, therefore specify explicitly
TST_NEEDS_TMPDIR=1

. tst_test.sh

SYSFS="/sys"
UMOUNT=
TST_FS_TYPE="ext3"

check_ima_policy()
{
	local policy="$1"
	local i

	grep -q "ima_$policy" /proc/cmdline && return
	for i in $(cat /proc/cmdline); do
		if echo "$i" | grep -q '^ima_policy='; then
			echo "$i" | grep -q -e "|[ ]*$policy" -e "$policy[ ]*|" -e "=$policy" && return
		fi
	done
	tst_brk TCONF "IMA measurement tests require builtin IMA $policy policy (e.g. ima_policy=$policy kernel parameter)"
}

mount_helper()
{
	local type="$1"
	local default_dir="$2"
	local dir

	dir="$(grep ^$type /proc/mounts | cut -d ' ' -f2 | head -1)"
	[ -n "$dir" ] && { echo "$dir"; return; }

	if ! mkdir -p $default_dir; then
		tst_brk TBROK "failed to create $default_dir"
	fi
	if ! mount -t $type $type $default_dir; then
		tst_brk TBROK "failed to mount $type"
	fi
	UMOUNT="$default_dir $UMOUNT"
	echo $default_dir
}

mount_loop_device()
{
	local ret

	tst_mkfs
	tst_mount
	cd $TST_MNTPOINT
}

print_ima_config()
{
	local config="/boot/config-$(uname -r)"
	local i

	if [ -r "$config" ]; then
		tst_res TINFO "IMA kernel config:"
		for i in $(grep ^CONFIG_IMA $config); do
			tst_res TINFO "$i"
		done
	fi

	tst_res TINFO "/proc/cmdline: $(cat /proc/cmdline)"
}

ima_setup()
{
	SECURITYFS="$(mount_helper securityfs $SYSFS/kernel/security)"

	IMA_DIR="$SECURITYFS/ima"
	[ -d "$IMA_DIR" ] || tst_brk TCONF "IMA not enabled in kernel"
	ASCII_MEASUREMENTS="$IMA_DIR/ascii_runtime_measurements"
	BINARY_MEASUREMENTS="$IMA_DIR/binary_runtime_measurements"

	print_ima_config

	if [ "$TST_NEEDS_DEVICE" = 1 ]; then
		tst_res TINFO "\$TMPDIR is on tmpfs => run on loop device"
		mount_loop_device
	fi

	[ -n "$TST_SETUP_CALLER" ] && $TST_SETUP_CALLER
}

ima_cleanup()
{
	local dir

	[ -n "$TST_CLEANUP_CALLER" ] && $TST_CLEANUP_CALLER

	for dir in $UMOUNT; do
		umount $dir
	done

	if [ "$TST_NEEDS_DEVICE" = 1 ]; then
		cd $TST_TMPDIR
		tst_umount
	fi
}

set_digest_index()
{
	DIGEST_INDEX=

	local template="$(tail -1 $ASCII_MEASUREMENTS | cut -d' ' -f 3)"
	local i word

	# parse digest index
	# https://www.kernel.org/doc/html/latest/security/IMA-templates.html#use
	case "$template" in
	ima|ima-ng|ima-sig) DIGEST_INDEX=4 ;;
	*)
		# using ima_template_fmt kernel parameter
		local IFS="|"
		i=4
		for word in $template; do
			if [ "$word" = 'd' -o "$word" = 'd-ng' ]; then
				DIGEST_INDEX=$i
				break
			fi
			i=$((i+1))
		done
	esac

	[ -z "$DIGEST_INDEX" ] && tst_brk TCONF \
		"Cannot find digest index (template: '$template')"
}

get_algorithm_digest()
{
	local line="$1"
	local delimiter=':'

	tst_res TINFO "measurement record: '$line'"

	DIGEST=$(echo "$line" | cut -d' ' -f $DIGEST_INDEX)
	if [ -z "$DIGEST" ]; then
		tst_res TFAIL "cannot find digest (index: $DIGEST_INDEX)"
		return
	fi

	if [ "${DIGEST#*$delimiter}" != "$DIGEST" ]; then
		ALGORITHM=$(echo "$DIGEST" | cut -d $delimiter -f 1)
		DIGEST=$(echo "$DIGEST" | cut -d $delimiter -f 2)
	else
		case "${#DIGEST}" in
		32) ALGORITHM="md5" ;;
		40) ALGORITHM="sha1" ;;
		*)
			tst_res TFAIL "algorithm must be either md5 or sha1 (digest: '$DIGEST')"
			return ;;
		esac
	fi
	if [ -z "$ALGORITHM" ]; then
		tst_res TFAIL "cannot find algorithm"
		return
	fi
	if [ -z "$DIGEST" ]; then
		tst_res TFAIL "cannot find digest"
		return
	fi
}

# loop device is needed to use only for tmpfs
TMPDIR="${TMPDIR:-/tmp}"
if [ "$(df -T $TMPDIR | tail -1 | awk '{print $2}')" != "tmpfs" -a -n "$TST_NEEDS_DEVICE" ]; then
	unset TST_NEEDS_DEVICE
fi
