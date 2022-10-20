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
    RUSER=${RUSER:-root}

}

do_test()
{
    case $1 in
    1) test_get binary;;
    2) test_get ascii;;
    3) test_put binary;;
    4) test_put ascii;;
    esac
}

list_files()
{
    case $1 in
    binary) echo 'ascii.sm ascii.med ascii.lg ascii.jmb';;
    ascii) echo 'bin.sm bin.med bin.lg bin.jmb';;
    esac
}

test_get()
{
    local sum1 sum2

    for file in $(list_files $1); do
        {
            echo user $RUSER $PASSWD
            echo $1
            echo cd $TST_NET_DATAROOT
            echo get $file
            echo quit
        } | ftp -nv $RHOST

        sum1="$(ls -l $file  | awk '{print $5}')"
        sum2="$(ls -l $TST_NET_DATAROOT/$file | awk '{print $5}')"
        rm -f $file
        EXPECT_PASS "[ '$sum1' = '$sum2' ]"
    done
}

test_put()
{
    local sum1 sum2

    for file in $(list_files $1); do
        {
            echo user $RUSER $PASSWD
            echo lcd $TST_NET_DATAROOT
            echo $1
            echo cd $TST_TMPDIR
            echo post $file
            echo quit
        } | ftp -nv $RHOST

        sum1="$(tst_rhost_run -c "sum $TST_TMPDIR/$file" -s | awk '{print $1}')"
        sum2="$(sum $TST_NET_DATAROOT/$file | awk '{print $1}')"
        tst_rhost_run -c "rm -f $TST_TMPDIR/$file"
        EXPECT_PASS "[ '$sum1' = '$sum2' ]"
    done
}

. tst_net.sh

tst_run
