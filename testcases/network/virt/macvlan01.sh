#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2018 Petr Vorel <pvorel@suse.cz>
# Copyright (c) 2015 Oracle and/or its affiliates. All Rights Reserved.
# Author: Alexey Kodanev <alexey.kodanev@oracle.com>
#
# Local test, check if we can create and then delete macvlan
# interface multiple times.

virt_type="macvlan"
options="mode private,mode vepa,mode bridge,mode passthru"
TST_CNT=4
TST_TESTFUNC=do_test
. virt_lib.sh

do_test()
{
	virt_test_02 "$options"
}

tst_run
