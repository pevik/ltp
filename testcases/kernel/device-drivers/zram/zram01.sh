#!/bin/sh
# Copyright (c) 2015 Oracle and/or its affiliates. All Rights Reserved.
# Copyright (c) 2019-2026 Petr Vorel <pvorel@suse.cz>
# Author: Alexey Kodanev <alexey.kodanev@oracle.com>
#
# Test creates zram device with a filesystem on it.
# It fills the device with zeros and checks that compression works.

TST_CNT=7
TST_TESTFUNC="do_test"
TST_NEEDS_CMDS="awk bc dd"
TST_SETUP="setup"
TST_OPTS="f:"
TST_USAGE="usage"
TST_PARSE_ARGS="parse_args"

usage()
{
	cat << EOF
usage: $0 [-f <ext2|ext3|ext4|xfs|btrfs|...>]

OPTIONS
-f	Specify the filesystem to be tested. If not specified,
	\$LTP_DEV_FS_TYPE or ext2 is used.
EOF
}

parse_args()
{
	TST_FS_TYPE="$2"
}

check_space_for_fs()
{
	local fs="$1"
	local ram_size

	ram_size=$(awk '/MemTotal:/ {print $2}' /proc/meminfo)
	if [ "$ram_size" -lt 1048576 ]; then
		tst_brk TCONF "not enough space for $fs"
	fi
}

# Parameters for zram device.
# NOTE about size:
# The zram sysfs node 'disksize' value can be either in bytes,
# or you can use mem suffixes. But in some old kernels, mem
# suffixes are not supported, for example, in RHEL6.6GA's kernel
# layer, it uses strict_strtoull() to parse disksize which does
# not support mem suffixes, in some newer kernels, they use
# memparse() which supports mem suffixes. So here we just use
# bytes to make sure everything works correctly.
initialize_vars()
{
	local fs="${TST_FS_TYPE:-${LTP_DEV_FS_TYPE:-ext2}}"
	local size="26214400"
	local limit="25M"

	[ "$fs" = "tmpfs" ] && tst_brk TCONF "tmpfs is not supported"

	tst_supported_fs "$fs" || \
		tst_brk TCONF "filesystem $fs is not supported"

	if [ "$fs" = "btrfs" -o "$fs" = "xfs" ]; then
		check_space_for_fs "$fs"

		if [ "$fs" = "btrfs" ]; then
			size="402653184"
		elif [ "$fs" = "xfs" ]; then
			size=314572800
		fi
		limit="$((size/1024/1024))M"
	fi

	tst_res TINFO "testing $fs filesystem"

	dev_num=1
	zram_filesystems="$fs"
	zram_mem_limits="$limit"
	zram_sizes="$size"
	zram_max_streams="2"
}

setup()
{
	initialize_vars
	zram_load
}

zram_makefs()
{
	local i=$dev_start
	local fs

	for fs in $zram_filesystems; do
		tst_res TINFO "make $fs filesystem on /dev/zram$i"
		mkfs.$fs /dev/zram$i > err.log 2>&1
		if [ $? -ne 0 ]; then
			cat err.log
			tst_res TFAIL "Failed to make $fs on /dev/zram$i"
			tst_brk TBROK "Can't continue with mounting the FS"
		fi

		i=$(($i + 1))
	done

	tst_res TPASS "zram_makefs succeeded"
}

zram_mount()
{
	local i=$dev_start
	local fs

	for fs in $zram_filesystems; do
		tst_res TINFO "mount /dev/zram$i ($fs)"
		mkdir zram$i
		ROD mount -t "$fs" /dev/zram$i zram$i
		dev_mounted=$i
		i=$((i + 1))
	done

	tst_res TPASS "mount of zram device(s) succeeded"
}

read_mem_used_total()
{
	echo $(awk '{print $3}' $1)
}

# Reads /sys/block/zram*/mm_stat until mem_used_total is not 0.
check_read_mem_used_total()
{
	local file="$1"
	local mem_used_total

	tst_res TINFO "$file"
	cat $file >&2

	mem_used_total=$(read_mem_used_total $file)
	[ "$mem_used_total" -eq 0 ] && return 1

	return 0
}

zram_fill_fs()
{
	local mem_used_total
	local b i r v

	for i in $(seq $dev_start $dev_end); do
		tst_res TINFO "filling zram$i (it can take long time)"
		b=0
		while true; do
			dd conv=notrunc if=/dev/zero of=zram${i}/file \
				oflag=append count=1 bs=1024 status=none \
				>/dev/null 2>err.txt || break
			b=$(($b + 1))
		done
		if [ $b -eq 0 ]; then
			[ -s err.txt ] && tst_res TWARN "dd error: $(cat err.txt)"
			tst_brk TBROK "cannot fill zram $i"
		fi
		tst_res TPASS "zram$i was filled with '$b' KB"

		if [ ! -f "/sys/block/zram$i/mm_stat" ]; then
			if [ $i -eq 0 ]; then
				tst_res TCONF "zram compression ratio test requires zram mm_stat sysfs file"
			fi

			continue
		fi

		TST_RETRY_FN_EXP_BACKOFF "check_read_mem_used_total /sys/block/zram$i/mm_stat" 0 10
		mem_used_total=$(read_mem_used_total /sys/block/zram$i/mm_stat)
		tst_res TINFO "mem_used_total: $mem_used_total"

		v=$((100 * 1024 * $b / $mem_used_total))
		r=$(echo "scale=2; $v / 100 " | bc)

		if [ "$v" -lt 100 ]; then
			tst_res TFAIL "compression ratio: $r:1"
			break
		fi

		tst_res TPASS "compression ratio: $r:1"
	done
}

do_test()
{
	case $1 in
	 1) zram_max_streams;;
	 2) zram_compress_alg;;
	 3) zram_set_disksizes;;
	 4) zram_set_memlimit;;
	 5) zram_makefs;;
	 6) zram_mount;;
	 7) zram_fill_fs;;
	esac
}

. zram_lib.sh
tst_run
