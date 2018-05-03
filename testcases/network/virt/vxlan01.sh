#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2018 Petr Vorel <pvorel@suse.cz>
# Copyright (c) 2014-2015 Oracle and/or its affiliates. All Rights Reserved.
# Author: Alexey Kodanev <alexey.kodanev@oracle.com>
#
# Local test, check if we can create multiple VXLAN interfaces.

virt_type="vxlan"
start_id=16700000

options="l2miss l3miss,norsc nolearning noproxy,\
ttl 0x01 tos 0x01,ttl 0xff tos 0xff,gbp"
TST_CNT=5
TST_TESTFUNC=do_test
. virt_lib.sh

do_test()
{
	virt_test_01 "$options"
}

tst_run
