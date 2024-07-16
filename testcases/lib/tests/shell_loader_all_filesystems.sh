#!/bin/sh
#
# needs_root=1
# mount_device=1
# all_filesystems=1

TST_MNTPOINT=ltp_mntpoint

. tst_env.sh

tst_res TINFO "IN shell"

mounted=$(grep $TST_MNTPOINT /proc/mounts)

if [ -n "$mounted" ]; then
	device=$(echo $mounted |cut -d' ' -f 1)
	path=$(echo $mounted |cut -d' ' -f 2)

	tst_res TPASS "$device mounted at $path"
else
	tst_res TFAIL "Device not mounted!"
fi
