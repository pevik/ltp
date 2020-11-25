#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2006 International Business Machines  Corp.
# Copyright (c) 2020 Joerg Vehlow <joerg.vehlow@aox-tech.de>
# Author: Mitsuru Chinen <mitch@jp.ibm.com>
#
# Verify the kernel is not crashed when the route is modified by
# ICMP Redirects frequently

TST_SETUP=do_setup
TST_CLEANUP=do_cleanup
TST_TESTFUNC=do_test
TST_NEEDS_ROOT=1
TST_NEEDS_CMDS="ip"

. tst_net.sh

DST_HOST=
DST_PORT="7"

do_setup()
{
	local lhost_ifname=$(tst_iface lhost)
	local rhost_ifname=$(tst_iface rhost)
	local rhost_net="$(tst_ipaddr_un -p 1)"

	DST_HOST="$(tst_ipaddr_un 1 5)"

	# Remove the link-local address of the remote host
	tst_rhost_run -s -c "ip addr flush dev $rhost_ifname"

	# Add route to the initial gateway
	ip route add $rhost_net dev $lhost_ifname

	# Make sure the sysctl value is set for accepting the redirect
	sysctl -w net.ipv${TST_IPVER}.conf.${lhost_ifname}.accept_redirects=1 > /dev/null
	[ ! "$TST_IPV6" ] && sysctl -w net.ipv4.conf.${lhost_ifname}.secure_redirects=0 > /dev/null

	tst_rhost_run -s -c "ns-icmp_redirector -I $rhost_ifname -b"
}

do_cleanup()
{
	tst_rhost_run -c "killall -SIGHUP ns-icmp_redirector"
}

do_test()
{
	tst_res TINFO "modify route by ICMP Redirects frequently"

	local cnt=0
	while [ $cnt -lt $NS_TIMES ]; do
		ns-udpsender -f $TST_IPVER -D $DST_HOST -p $DST_PORT -o -s 8
		if [ $? -ne 0 ]; then
			tst_res TBROK "failed to run udp packet sender"
		fi
		n=$((n+1))
	done

	tst_res TPASS "test finished successfully"
}

tst_run
