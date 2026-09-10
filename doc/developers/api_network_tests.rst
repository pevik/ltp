.. SPDX-License-Identifier: GPL-2.0-or-later

Developing using network API
============================

Overview
--------

LTP network tests support both single-host and two-host test configurations.
Network stress tests and environment parameters are defined in
:shell_lib:`tst_net.sh` and documented in
:master:`testcases/network/README.md`.

Single-host configuration
~~~~~~~~~~~~~~~~~~~~~~~~~

Single-host is the default configuration when the ``RHOST`` environment
variable is not defined. LTP creates an ``ltp_ns`` network namespace and
configures a ``veth`` pair according to LTP network environment variables.

Two-host configuration
~~~~~~~~~~~~~~~~~~~~~~

This setup requires the ``RHOST`` environment variable to be set to the
hostname or IP address of the remote test machine, along with passwordless
SSH access configured for root. Both machines must have LTP installed in
the same location.

For more details on network test setup, server service dependencies, and
running the tests with ``network.sh``, see
:master:`testcases/network/README.md`.
