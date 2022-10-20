#!/bin/sh
#
#   Copyright (c) International Business Machines  Corp., 2000
#
#   This program is free software;  you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY;  without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
#   the GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program;  if not, write to the Free Software
#   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
#
#
#
#  FILE   : ftp
#
#  PURPOSE: To test the basic functionality of the `ftp` command.
#
#  SETUP: The home directory of root on the machine exported as "RHOST"
#         MUST have a ".rhosts" file with the hostname of the machine
#         where the test is executed. Also, both machines MUST have
#         the same path configuration for the test for proper test data
#         file transfers. The PASSWD variable should also be set to root's
#	  login password.
#
#  HISTORY:
#    06/06/03 Manoj Iyer manjo@mail.utexas.edu
#      - Modified to use LTP test harness APIs
#    03/01 Robbie Williamson (robbiew@us.ibm.com)
#      -Ported
#
#
#-----------------------------------------------------------------------
#
#----------------------------------------------------------------------

TST_TESTFUNC=do_test
TST_SETUP=do_setup
TST_NEEDS_CMDS='awk ftp'
TST_NEEDS_TMPDIR=1

do_setup()
{

    TC=ftp
    ASCII_FILES=${ASCII_FILES:-"ascii.sm ascii.med ascii.lg ascii.jmb"}
    BIN_FILES=${BIN_FILES:-"bin.sm bin.med bin.lg bin.jmb"}

    RUSER=${RUSER:-root}

}

do_test()
{
    local sum1 sum2

    for i in binary ascii; do

        if [ $i = "binary" ]; then
            FILES=$BIN_FILES
        fi
        if [ $i = "ascii" ]; then
            FILES=$ASCII_FILES
        fi
        for j in $FILES; do

            for a in get put; do
                if [ $a = "get" ]; then
                    {
                        echo user $RUSER $PASSWD
                        echo $i
                        echo cd $TST_NET_DATAROOT
                        echo $a $j
                        echo quit
                    } | ftp -nv $RHOST
                    sum1="$(ls -l $j  | awk '{print $5}')"
                    sum2="$(ls -l $TST_NET_DATAROOT/$j | awk '{print $5}')"
                    rm -f $j
                else
                    {
                        echo user $RUSER $PASSWD
                        echo lcd $TST_NET_DATAROOT
                        echo $i
                        echo cd $TST_TMPDIR
                        echo $a $j
                        echo quit
                    } | ftp -nv $RHOST
                    sum1="$(tst_rhost_run -c "sum $TST_TMPDIR/$j" -s | awk '{print $1}')"
                    sum2="$(sum $TST_NET_DATAROOT/$j | awk '{print $1}')"
                    tst_rhost_run -c "rm -f $TST_TMPDIR/$j"
                fi

                EXPECT_PASS "[ '$sum1' = '$sum2' ]"
            done
        done
    done
}

. tst_net.sh

tst_run
