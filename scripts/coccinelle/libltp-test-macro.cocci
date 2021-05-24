// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2021 SUSE LLC  <rpalethorpe@suse.com>

// The TEST macro should not be used in the library because it sets
// TST_RET and TST_ERR which are global variables. The test author
// only expects these to be changed if *they* call TEST directly.

// Set with -D fix
virtual fix
// Set with -D report
virtual report

// Find all positions where TEST is _used_.
@ find_use exists @
position p;
@@

 TEST@p(...);

// Print the position of the TEST usage
@initialize:python depends on report@
@@

import json

errors = []

@script:python depends on report@
p << find_use.p;
@@

msg = {
    'msg': "TEST macro use in library",
    'file': p[0].file,
    'line': p[0].line,
    'col': p[0].column,
}

errors.append(msg)

@finalize:python depends on report@
@@

msgs = {
     'check_script': 'coccinelle/libltp-test-macro.cocci',
     'errors': errors,
}

print(json.dumps(msgs, indent=2))

// Below are rules which will create a patch to replace TEST usage
// It assumes we can use the ret var without conflicts

// Fix all references to the variables TEST modifies when they occur in a
// function where TEST was used.
@ depends on fix && find_use @
@@

(
- TST_RET
+ ret
|
- TST_ERR
+ errno
|
- TTERRNO
+ TERRNO
)

// Replace TEST in all functions where it occurs only at the start. It
// is slightly complicated by adding a newline if a statement appears
// on the line after TEST(). It is not clear to me what the rules are
// for matching whitespace as it has no semantic meaning, but this
// appears to work.
@ depends on fix @
identifier fn;
expression tested_expr;
statement st;
@@

  fn (...)
  {
- 	TEST(tested_expr);
+	const long ret = tested_expr;
(
+
	st
|

)
	... when != TEST(...)
  }

// Replace TEST in all functions where it occurs at the start
// Functions where it *only* occurs at the start were handled above
@ depends on fix @
identifier fn;
expression tested_expr;
statement st;
@@

  fn (...)
  {
- 	TEST(tested_expr);
+	long ret = tested_expr;
(
+
	st
|

)
	...
  }

// Add ret var at the start of a function where TEST occurs and there
// is not already a ret declaration
@ depends on fix exists @
identifier fn;
@@

  fn (...)
  {
+	long ret;
	... when != long ret;

	TEST(...)
	...
  }

// Replace any remaining occurances of TEST
@ depends on fix @
expression tested_expr;
@@

- 	TEST(tested_expr);
+	ret = tested_expr;
