#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2006 International Business Machines  Corp.
# Copyright (c) 2020 Joerg Vehlow <joerg.vehlow@aox-tech.de>
# Author: Mitsuru Chinen <mitch@jp.ibm.com>
#
# Verify that the kernel is not crashed when joining multiple multicast
# groups on separate sockets, then receiving a large number of UDP
# packets at each socket

TST_NEEDS_ROOT=1
. mcast-lib.sh

do_setup()
{
	mcast_setup $n_NORMAL
	MCAST_LCMD=ns-mcast_receiver
	MCAST_RCMD=ns-udpsender
}

do_test()
{
	tst_res TINFO "Verify that the kernel is not crashed when joining $MCASTNUM_NORMAL IPv${TST_IPVER} multicast groups on separate sockets, then receiving a large number of UDP packets at each socket in $NS_DURATION seconds"

	local n=0
	while [ $n -lt $MCASTNUM_NORMAL ]; do
		# Define the multicast address
		if [ "$TST_IPV6" ]; then
		local n_hex=$(printf "%x" $n)
		local addr=${MCAST_IPV6_ADDR_PREFIX}:${n_hex}
		else
			local x=$((n / 254))
			local y=$((n % 254 + 1))
			local addr=${MCAST_IPV4_ADDR_PREFIX}.${x}.${y}
		fi

		local port=$(tst_get_unused_port ipv${TST_IPVER} dgram)
		[ $? -ne 0 ] && tst_brk TBROK "No free udp port available."
		tst_res TINFO Port: $port

		# Run a receiver
		ROD ns-mcast_receiver -f $TST_IPVER -I $(tst_iface lhost) -m $addr -p $port -b

		# Run a sender
		tst_rhost_run -s -c "ns-udpsender -D $addr -f $TST_IPVER -p $port -m -I $(tst_iface rhost) -b -t $NS_DURATION"

		: $((n += 1))
	done

	sleep $NS_DURATION

	tst_res TPASS "Test finished successfully."
}

tst_run
