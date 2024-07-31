#!/bin/sh
#
# This test has wrong metadata and should not be run
#
# TEST = {
#  "needs_tmpdir": 42,
# }
#

. tst_loader.sh

tst_res TFAIL "Shell loader should TBROK the test"
