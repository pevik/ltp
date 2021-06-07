// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2021 SUSE LLC  <rpalethorpe@suse.com>

// The TEST macro should not be used in the library because it sets
// TST_RET and TST_ERR which are global variables. The test author
// only expects these to be changed if *they* call TEST directly.

// Find all positions where TEST's variables are used
@ find_use exists @
identifier tst_var =~ "TST_(ERR|RET)";
position p;
expression E;
@@

(
* tst_var@p
|
* TTERRNO@p | E
)
