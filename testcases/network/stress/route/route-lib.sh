#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2019 Petr Vorel <pvorel@suse.cz>

TST_NEEDS_ROOT=1
TST_SETUP="setup"
TST_CLEANUP="route_cleanup"
TST_NEEDS_CMDS="ip"

. tst_net.sh

MAX_IP=${MAX_IP:-5}

check_max_ip()
{
	local max_ip_limit=254
	[ "$TST_IPV6" ] && max_ip_limit=65534

	tst_is_int $MAX_IP || tst_brk TBROK "\$MAX_IP not int ($MAX_IP)"
	[ "$MAX_IP" -gt $max_ip_limit ] && MAX_IP=$max_ip_limit
}

setup_gw()
{
	tst_res TINFO "change IPv$TST_IPVER route gateway $NS_TIMES times"

	rt="$(tst_ipaddr_un -p 0 0)"
	lhost="$(tst_ipaddr_un 1 1)"
	rhost="$(tst_ipaddr_un 0 1)"
	tst_add_ipaddr -s -q -a $lhost
	tst_add_ipaddr -s -q -a $rhost rhost
}


test_netlink()
{
	local ip_flag
	[ "$TST_IPV6" ] && ip_flag="-6"

	local port=$(tst_rhost_run -s -c "tst_get_unused_port ipv${TST_IPVER} dgram")

	EXPECT_PASS route-change-netlink -c $NS_TIMES -d $(tst_iface) $ip_flag -p $port $ROUTE_CHANGE_NETLINK_PARAMS
}

route_cleanup()
{
	tst_restore_ipaddr
	tst_restore_ipaddr rhost
}
