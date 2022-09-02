#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) Linux Test Project, 2015-2022
# Copyright (c) 2015 Fujitsu Ltd.
# Author: Guangwen Feng <fenggw-fnst@cn.fujitsu.com>
#
# Test mkfs command with some basic options.

TST_ALL_FILESYSTEMS=1
TST_CNT=5
TST_SETUP=setup
TST_TESTFUNC=test
TST_NEEDS_ROOT=1
TST_NEEDS_CMDS="blkid df"

setup()
{
	if [ -n "$TST_FS_TYPE" ]; then
		tst_require_cmds mkfs.$TST_FS_TYPE
	fi
}

mkfs_verify_type()
{
	if [ -z "$1" ]; then
		blkid $2 -t TYPE="$TST_FS_TYPE" >/dev/null
	else
		if [ "$1" = "msdos" ]; then
			blkid $2 -t TYPE="vfat" >/dev/null
		else
			blkid $2 -t TYPE="$1" >/dev/null
		fi
	fi
}

mkfs_verify_size()
{
	tst_mount
	local blocknum=`df -P -B 1k $TST_MNTPOINT | tail -n1 | awk '{print $2}'`
	tst_umount

	if [ $blocknum -gt "$2" ]; then
		return 1
	fi

	# Size argument in mkfs.ntfs denotes number-of-sectors which is 512bytes,
	# 1k-block size should be devided by this argument for ntfs verification.
	if [ "$1" = "ntfs" ]; then
		local rate=1024/512
		if [ $blocknum -lt "$(($2/$rate*8/10))" ]; then
			return 1
		fi
	else
		if [ $blocknum -lt "$(($2*8/10))" ]; then
			return 1
		fi
	fi

	return 0
}

mkfs_test()
{
	local mkfs_op=$1
	local fs_type=$2
	local fs_op=$3
	local device=$4
	local size=$5

	if [ -n "$fs_type" ]; then
		mkfs_op="-t $fs_type"
	fi

	if [ "$fs_type" = "xfs" ] || [ "$fs_type" = "btrfs" ]; then
		fs_op="$fs_op -f"
	fi
	if [ "$fs_type" = "ext3" ] || [ "$fs_type" = "ext4" ]; then
		fs_op="$fs_op -b 1024"
	fi

	local mkfs_cmd="mkfs $mkfs_op $fs_op $device $size"

	echo $fs_op | grep -q "\-c"
	if [ $? -eq 0 ] && [ "$fs_type" = "ntfs" ]; then
		tst_res TCONF "'$mkfs_cmd' not supported."
		return
	fi

	if [ -n "$size" ]; then
		if [ "$fs_type" = "xfs" ] || [ "$fs_type" = "btrfs" ]; then
			tst_res TCONF "'$mkfs_cmd' not supported."
			return
		fi
	fi

	$mkfs_cmd >temp 2>&1
	if [ $? -ne 0 ]; then
		grep -q -E "unknown option | invalid option" temp
		if [ $? -eq 0 ]; then
			tst_res TCONF "'$mkfs_cmd' not supported."
			return
		else
			tst_res TFAIL "'$mkfs_cmd' failed."
			cat temp
			return
		fi
	fi

	if [ -n "$device" ]; then
		mkfs_verify_type "$fs_type" "$device"
		if [ $? -ne 0 ]; then
			tst_res TFAIL "'$mkfs_cmd' failed, unexpected type."
			cat temp
			return
		fi
	fi

	if [ -n "$size" ]; then
		mkfs_verify_size "$fs_type" "$size"
		if [ $? -ne 0 ]; then
			tst_res TFAIL "'$mkfs_cmd' failed, unexpected size."
			cat temp
			return
		fi
	fi

	tst_res TPASS "'$mkfs_cmd' passed."
}

test1()
{
	mkfs_test "" "$TST_FS_TYPE" "" "$TST_DEVICE"
}

test2()
{
	mkfs_test "" "$TST_FS_TYPE" "" "$TST_DEVICE" "16000"
}

test3()
{
	mkfs_test "" "$TST_FS_TYPE" "-c" "$TST_DEVICE"
}

test4()
{
	mkfs_test "-V"
}

test5()
{
	mkfs_test "-h"
}

. tst_test.sh
tst_run
