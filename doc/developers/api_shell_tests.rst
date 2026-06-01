.. SPDX-License-Identifier: GPL-2.0-or-later

LTP shell API
=============

Shell API overview
------------------

First lines of the shell test should be a shebang, a license, and copyrights.

.. code-block:: shell

   #!/bin/sh
   # SPDX-License-Identifier: GPL-2.0-or-later
   # Copyright 2099 Foo Bar <foo.bar@example.org>

A documentation comment block formatted in ReStructuredText should follow right
after these lines. This comment is parsed and exported into the LTP
documentation at https://linux-test-project.readthedocs.io/en/latest/users/test_catalog.html

.. code-block:: shell

   # ---
   # doc
   # Test for a foo bar.
   #
   # This test is testing foo by checking that bar is doing xyz.
   # ---

The shell loader test library uses the :doc:`../developers/api_c_tests`
internally by parsing a special JSON formatted comment and
initializing it accordingly. The JSON format is nearly 1:1 serialization of the
:ref:`struct tst_test` into a JSON. The environment must be always preset even
when it's empty.

.. code-block:: shell

   # ---
   # env
   # {
   #  "needs_root": true,
   #  "needs_tmpdir": true,
   #  "needs_kconfigs": ["CONFIG_NUMA=y"],
   #  "tags": {
   #   ["linux-git", "432fd03240fa"]
   #  }
   # }

After the documentation and environment has been laid out we finally import the
:shell_lib:`tst_loader.sh`. This will, among other things, start the
:shell_lib:`tst_run_shell.c` binary, that will parse the shell test environment
comment and initialize the C test library accordingly.

.. code-block:: shell

   . tst_loader.sh

At this point everything has been set up and we can finally write the test
function. The test results are reported by the usual functions :ref:`tst_res` and
:ref:`tst_brk`. As in the C API these functions store results into a piece of shared
memory as soon as they return so there is no need to propagate results event
from child processes.

.. code-block:: shell

   tst_test()
   {
        tst_res TPASS "Bar enabled Foo"
   }

In order for the test to be actually executed the very last line of the script
must source the :shell_lib:`tst_run.sh` script.

.. code-block:: shell

   . tst_run.sh

In order to run a test from a LTP tree a few directories has to be added to the
`$PATH`. Note that the number of `../` may depend on the depth of the current
directory relative to the LTP root.

.. code-block:: shell

   $ PATH=$PATH:$PWD:$PWD/../../lib/ ./foo01.sh

Test setup and cleanup
----------------------

The test setup and cleanup functions are optional and passed via variables.
Similarly to the C API the setup is executed exactly once at the start of the
test and the test cleanup is executed at the test end or when test was
interrupted by :ref:`tst_brk`.

   .. literalinclude:: ../../testcases/lib/tests/shell_loader_setup_cleanup.sh
      :language: shell

Shell API variables
-------------------

The following variables are available to shell tests. Variables marked
*input* are set by the test before sourcing ``tst_run.sh``; variables
marked *output* are set by the library for tests to read.

Test definition (input)
~~~~~~~~~~~~~~~~~~~~~~~

``TST_TESTFUNC``
    Name of the test function (required).

``TST_CNT``
    Number of test cases. When set the test function is called
    ``TST_CNT`` times with a counter argument.

``TST_SETUP``
    Name of the setup function (called once before tests).

``TST_CLEANUP``
    Name of the cleanup function (called once after tests or on
    ``tst_brk``).

``TST_OPTS``
    Extra getopts option string.

``TST_PARSE_ARGS``
    Name of a function to parse extra options from ``TST_OPTS``.

``TST_USAGE``
    Name of a function printing extra usage information.

``TST_POS_ARGS``
    Number of expected positional arguments.

``TST_TEST_DATA``
    Space-separated data passed as a second argument to the test
    function.

``TST_TEST_DATA_IFS``
    Custom delimiter for ``TST_TEST_DATA`` (default: space).

Requirements (input)
~~~~~~~~~~~~~~~~~~~~

``TST_NEEDS_ROOT``
    Set to 1 to require root privileges.

``TST_NEEDS_TMPDIR``
    Set to 1 to create a temporary directory.

``TST_NEEDS_DEVICE``
    Set to 1 to prepare a block device.

``TST_NEEDS_CMDS``
    Space-separated list of required commands.

``TST_NEEDS_MODULE``
    Kernel module name that must be loadable.

``TST_NEEDS_DRIVERS``
    Space-separated list of required kernel drivers.

``TST_NEEDS_KCONFIGS``
    Space-separated list of required kernel config options
    (e.g. ``CONFIG_NUMA=y``).

``TST_NEEDS_KCONFIGS_IFS``
    Custom delimiter for ``TST_NEEDS_KCONFIGS`` (default: comma).

``TST_NEEDS_CHECKPOINTS``
    Set to 1 to enable checkpoint support.

``TST_MIN_KVER``
    Minimum kernel version string (e.g. ``4.18``).

Device and filesystem (input)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``TST_FORMAT_DEVICE``
    Set to 1 to format ``TST_DEVICE`` before the test.

``TST_MOUNT_DEVICE``
    Set to 1 to mount ``TST_DEVICE`` at ``TST_MNTPOINT``.

``TST_DEV_EXTRA_OPTS``
    Extra options passed to ``mkfs``.

``TST_DEV_FS_OPTS``
    Extra filesystem-specific ``mkfs`` options.

``TST_FS_TYPE``
    Filesystem type for formatting (default: ``ext2``).

``TST_DEVICE_SIZE``
    Device size in MB.

``TST_ALL_FILESYSTEMS``
    Set to 1 to repeat the test for each supported filesystem.

``TST_SKIP_FILESYSTEMS``
    Space-separated list of filesystems to skip.

``TST_MNT_PARAMS``
    Mount flags/options string.

Miscellaneous (input)
~~~~~~~~~~~~~~~~~~~~~

``TST_SKIP_IN_LOCKDOWN``
    Set to 1 to skip when kernel lockdown is active.

``TST_SKIP_IN_SECUREBOOT``
    Set to 1 to skip when Secure Boot is enabled.

``TST_TIMEOUT``
    Test timeout in seconds.

Output variables
~~~~~~~~~~~~~~~~

``TST_TMPDIR``
    Path to the temporary directory (when ``TST_NEEDS_TMPDIR=1``).

``TST_STARTWD``
    Original working directory before ``chdir`` to ``TST_TMPDIR``.

``TST_DEVICE``
    Block device path (when ``TST_NEEDS_DEVICE=1``).

``TST_MNTPOINT``
    Mount point path.

``TST_MODPATH``
    Path to the loaded kernel module.

Checkpoint functions
~~~~~~~~~~~~~~~~~~~~

``TST_CHECKPOINT_WAIT``
    Wait on checkpoint (argument: checkpoint id).

``TST_CHECKPOINT_WAKE``
    Wake one waiter on checkpoint (argument: checkpoint id).

``TST_CHECKPOINT_WAKE2``
    Wake multiple waiters (arguments: checkpoint id, count).

``TST_CHECKPOINT_WAKE_AND_WAIT``
    Wake one waiter and then wait (argument: checkpoint id).

Retry helpers
~~~~~~~~~~~~~

``TST_RETRY_FUNC``
    Retry a function until it succeeds or times out
    (arguments: function, expected value).

``TST_RETRY_FN_EXP_BACKOFF``
    Like ``TST_RETRY_FUNC`` with exponential backoff
    (arguments: function, expected value, max delay).
