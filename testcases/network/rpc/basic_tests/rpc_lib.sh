#!/bin/sh
# Copyright (c) 2020 Petr Vorel <pvorel@suse.cz>

. tst_net.sh

check_portmap_rpcbind()
{
	if pgrep portmap > /dev/null; then
		PORTMAPPER="portmap"
	else
		# In case of systemd socket activation, rpcbind could be
		# not started until somebody tries to connect to it's socket.
		#
		# To handle that case properly, run a client now.
		rpcinfo >/dev/null 2>&1

		pgrep rpcbind > /dev/null && PORTMAPPER="rpcbind" || \
			tst_brk TCONF "portmap or rpcbind is not running"
	fi
	tst_res TINFO "using $PORTMAPPER"
}
