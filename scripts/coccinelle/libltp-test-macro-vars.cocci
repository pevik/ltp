// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2021 SUSE LLC  <rpalethorpe@suse.com>

// The TEST macro should not be used in the library because it sets
// TST_RET and TST_ERR which are global variables. The test author
// only expects these to be changed if *they* call TEST directly.

// Set with -D fix
virtual fix
// Set with -D report
virtual report

// Find all positions where TEST's variables are used
@ find_use exists @
identifier tst_var =~ "TST_(ERR|RET)";
position p;
expression E;
@@

(
 tst_var@p
|
 TTERRNO@p | E
)

@initialize:python depends on report@
@@

import json

errors = []

@script:python depends on report@
p << find_use.p;
@@

msg = {
    'msg': "TEST macro variable use in library",
    'file': p[0].file,
    'line': p[0].line,
    'col': p[0].column,
}

errors.append(msg)

@finalize:python depends on report@
@@

msgs = {
     'check_script': 'coccinelle/libltp-test-macro-vars.cocci',
     'errors': errors,
}

print(json.dumps(msgs, indent=2))
