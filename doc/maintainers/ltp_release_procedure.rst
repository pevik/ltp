.. SPDX-License-Identifier: GPL-2.0-or-later

Release process
===============

Preparations
------------

The release procedure generally takes a few weeks. In the first week or two,
patches that should go into the release are reviewed and possibly merged. These
patches are either fixes or patches pointed out by the community.

Patch review, when finished, is followed by a git freeze, which is a period
where only fixes are pushed to the git. During that period community is
expected to run a LTP pre-release tests, reports problems, and/or send fixes to
the mailing list. In this period we are especially making sure that there are
no regressions in the test results on a wide range of distributions and
architectures.

Once the stabilization period has ended the time has finally come to proceed
with the release.

Prepare the release notes
-------------------------

Part of the preparation is also to write the release notes, which are then
added to the GitHub release and also sent as announcement to various mailing
lists (see below).

Have a look at `this release letter <https://lore.kernel.org/ltp/ZGNiQ1sMGvPU_ETp@yuki/>`_
to get the idea how it should look.

Tag the git and push changes to github
--------------------------------------

.. code-block:: bash

    cd ltp
    echo YYYYMMDD > VERSION
    git commit -S -s -m 'LTP YYYYMMDD' VERSION
    git tag -s -a YYYYMMDD -m 'LTP YYYYMMDD'
    git push origin master:master
    git push origin YYYYMMDD

The string ``YYYYMMDD`` should be substituted to the current date.

You can use :master:`tools/tag-release.sh` script to have the above automated
process. It allows you to verify the tag before pushing it and does other
checks.

.. code-block:: bash

    $ ./tools/tag-release.sh
    ===== git push =====
    new tag: 'YYYYMMDD', previous tag: '20230127'
    tag YYYYMMDD
    Tagger: Person-who-released LTP <foo@example.com>
    Date:   ...

    LTP YYYYMMDD
    -----BEGIN PGP SIGNATURE-----
    ...
    -----END PGP SIGNATURE-----

    commit 3ebc2dfa85c2445bb68d8c0d66e33c4da1e1b3a7
    gpg:                using RSA key ...
    ...
    Primary key fingerprint: ...
    Author: Person-who-released LTP <foo@example.com>
    Date:   ...

        LTP YYYYMMDD

        Signed-off-by: Person-who-released LTP <foo@example.com>

    diff --git a/VERSION b/VERSION
    index af4c41fec..ae488c0e7 100644
    --- a/VERSION
    +++ b/VERSION
    @@ -1 +1 @@
    -20230127
    +YYYYMMDD

    Please check tag and signature. Proceed? [N/y]: y
    Pushing changes to upstream git. Proceed? [N/y]: y
    ...
    To github.com:linux-test-project/ltp.git
     * [new tag]             YYYYMMDD -> YYYYMMDD

Prepare tarballs and metadata documentation
-------------------------------------------

The following procedure will show how to create the release archives and the
metadata documentation:

.. code-block:: bash

    # clone already clonned git repository to new folder
    cd ..
    git clone ltp ltp-full-YYYYMMDD
    cd ltp-full-YYYYMMDD

    # update all submodules
    git submodule update --init

    # Generate configure script
    make autotools

    # Generate tarballs
    cd ..
    tar -cjf ltp-full-YYYYMMDD.tar.bz2 ltp-full-YYYYMMDD --exclude .git
    tar -cJf ltp-full-YYYYMMDD.tar.xz ltp-full-YYYYMMDD --exclude .git

    # Generate checksums
    md5 ltp-full-YYYYMMDD.tar.xz > ltp-full-YYYYMMDD.tar.xz.md5
    sha1 ltp-full-YYYYMMDD.tar.xz > ltp-full-YYYYMMDD.tar.xz.sha1
    sha256sum ltp-full-YYYYMMDD.tar.xz > ltp-full-YYYYMMDD.tar.xz.sha256

You can use :master:`tools/create-tarballs-metadata.sh` script to have the above
procedure automated. All generated files are placed in the
``ltp-release-YYYYMMDD`` directory.

.. code-block:: bash

    $ ./tools/create-tarballs-metadata.sh
    ===== git clone =====
    Cloning into 'ltp-full-YYYYMMDD'...
    done.
    ===== Update submodules =====
    Submodule 'tools/kirk' (https://github.com/linux-test-project/kirk.git) registered for path 'tools/kirk'
    ...
    ===== Generate configure script =====
    sed -n '1{s:LTP-:m4_define([LTP_VERSION],[:;s:$:]):;p;q}' VERSION > m4/ltp-version.m4
    aclocal -I m4
    ...
    ===== Generate tarballs =====
    ===== Generate checksums =====
    ===== Generate metadata documentation =====
    checking for a BSD-compatible install... /usr/bin/install -c
    ...
    Generated files are in '/home/foo/ltp-release-YYYYMMDD', upload them to github

Upload the generated files to GitHub
------------------------------------

Go to :repo:`tags`. Click on ``Add release notes``.
There should be ``Attach binaries ...`` link at the bottom of the page.

Don't forget to upload checksums for the tarballs and metadata documentation
as well.

Send release announcement
-------------------------

The announcement is sent to:

* ltp at lists.linux.it (requires a subscription)
* linux-kernel at vger.kernel.org
* libc-alpha at sourceware.org (requires a subscription)
* valgrind-developers at lists.sourceforge.net (requires a subscription)

CCed to:

* lwn at lwn.net
* akpm at linux-foundation.org
* torvalds at linux-foundation.org
