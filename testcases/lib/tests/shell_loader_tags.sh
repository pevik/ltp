#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2024-2025 Cyril Hrubis <chrubis@suse.cz>
#
# ---
# env
# {
#  "tags": [
#   ["linux-git", "832478cd342ab"],
#   ["CVE", "2099-999"]
#  ]
# }
# ---

tst_test()
{
	tst_res TFAIL "Fails the test so that tags are shown."
}

. tst_loader.sh
