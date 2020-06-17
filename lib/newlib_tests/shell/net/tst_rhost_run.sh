#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2020 Petr Vorel <pvorel@suse.cz>

TST_TESTFUNC=do_test
PATH="$(dirname $0)/../../../../testcases/lib/:$PATH"
. tst_net.sh

do_test()
{
	local file="/etc/fstab"

	tst_rhost_run -d -c hostname

	tst_rhost_run -c "[ -f $file ]" || \
		tst_res TCONF "$file not found on rhost"

	tst_rhost_run -dsv -c "grep -q \"[^ ]\" $file"
	tst_rhost_run -dsv -c "grep -q '[^ ]' $file"

	tst_res TPASS "tst_rhost_run is working"
}

tst_run
