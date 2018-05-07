#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2016 Red Hat Inc.,  All Rights Reserved.
# Copyright (c) International Business Machines  Corp., 2005
# Author: Hangbin Liu <haliu@redhat.com>

TCID=${TCID:-icmp-uni-basic}
TST_TOTAL=1
TST_COUNT=1
TST_CLEANUP="tst_ipsec_cleanup"

. ipsec_lib.sh

LINK_NUM=${LINK_NUM:-0}
DO_IPSEC=${DO_IPSEC:-false}
[ -n "$IPSEC_MODE" -a -n "$IPSEC_PROTO" ] && DO_IPSEC=true || DO_IPSEC=false

# Test description
tst_resm TINFO "Sending ICMP messages with the following conditions"
tst_resm TINFO "- Version of IP is IPv${TST_IPV6:-4}"
tst_resm TINFO "- Size of packets are ( $IPSEC_SIZE_ARRAY )"

if $DO_IPSEC; then
	case $IPSEC_PROTO in
	ah)	tst_resm TINFO "- IPsec [ AH / $IPSEC_MODE ]" ;;
	esp)	tst_resm TINFO "- IPsec [ ESP / $IPSEC_MODE ]" ;;
	comp)	tst_resm TINFO "- IPcomp [ $IPSEC_MODE ]" ;;
	esac
fi

# name of interface of the local/remote host
lhost_ifname=$(tst_iface lhost $LINK_NUM)
rhost_ifname=$(tst_iface rhost $LINK_NUM)

lhost_addr=$(tst_ipaddr)
rhost_addr=$(tst_ipaddr rhost)

# Configure SAD/SPD
if $DO_IPSEC ; then
	tst_ipsec lhost $lhost_addr $rhost_addr
	tst_ipsec rhost $rhost_addr $lhost_addr
fi

PING_MAX="$IPSEC_REQUESTS"

tst_ping $lhost_ifname $rhost_addr $IPSEC_SIZE_ARRAY

tst_exit
