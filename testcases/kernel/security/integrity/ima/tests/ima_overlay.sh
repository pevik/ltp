#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Based on evm_overlay.sh testing environment

TST_SETUP="setup"
TST_CLEANUP="cleanup"
TST_NEEDS_DEVICE=1
TST_CNT=1
. ima_setup.sh

setup()
{
	[ -f "$IMA_POLICY" ] || tst_brk TCONF "IMA not enabled in kernel"

	cat "${IMA_POLICY}"

	lower="$TST_MNTPOINT/lower"
	upper="$TST_MNTPOINT/upper"
	work="$TST_MNTPOINT/work"
	merged="$TST_MNTPOINT/merged"
	mkdir -p $lower $upper $work $merged

	device_backup="$TST_DEVICE"
	TST_DEVICE="overlay"

	fs_type_backup="$TST_FS_TYPE"
	TST_FS_TYPE="overlay"

	mntpoint_backup="$TST_MNTPOINT"
	TST_MNTPOINT="$merged"

	params_backup="$TST_MNT_PARAMS"
	TST_MNT_PARAMS="-o lowerdir=$lower,upperdir=$upper,workdir=$work"

	UUID=$(lsblk "${device_backup}" -no UUID)
	echo "UUID: $UUID"
	# Requires ability to write and read policy
	echo "measure func=FILE_CHECK fsuuid=${UUID} mask=^MAY_READ" > ${IMA_POLICY}
	echo "measure func=FILE_CHECK fsname=overlay mask=^MAY_READ" > ${IMA_POLICY}
	tst_mount
	mounted=1
}

test1()
{
	# Don't hardcode the public and private keys, supply as a variable
	# expect public key to be suffixed with x509,
	# expect certificate to be suffixed with .pem
	local SIGNING_KEY=/home/mimi/src/kernel/linux-integrity/certs/signing_key
	local file="foo1.txt"

	UUID=$(lsblk "${device_backup}" -no UUID)
	echo "UUID: $UUID"

	tst_res TINFO "overwrite file in overlay"
	EXPECT_PASS echo lower \> $lower/$file
	stat $lower/$file

	Load public key on IMA keyring
	keyid="$(keyctl describe %keyring:.ima | cut -f1 -d ' ')"
	echo "keyid $keyid"
	evmctl import $SIGNING_KEY\.x509  $keyid

	evmctl ima_sign $lower/$file -k $SIGNING_KEY.pem
	getfattr -m ^security -e hex --dump $lower/$file
	evmctl ima_sign $merged/$file -k $SIGNING_KEY.pem
	getfattr -m ^security -e hex --dump $merged/$file

	echo "appraise func=FILE_CHECK fsuuid=${UUID} appraise_type=imasig" > ${IMA_POLICY}
	echo "appraise func=FILE_CHECK fsname=overlay appraise_type=imasig" > ${IMA_POLICY}

	EXPECT_PASS cat $lower/$file
	EXPECT_PASS cat $merged/$file
	bash

	EXPECT_FAIL echo overlay \> $merged/$file
	stat $merged/$file
}

cleanup()
{
	[ -n "$mounted" ] || return 0

	tst_umount $TST_DEVICE

	TST_DEVICE="$device_backup"
	TST_FS_TYPE="$fs_type_backup"
	TST_MNTPOINT="$mntpoint_backup"
	TST_MNT_PARAMS="$params_backup"
}

tst_run
set +x
