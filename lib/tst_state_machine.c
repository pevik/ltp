// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2022 SUSE LLC <rpalethorpe@suse.com>
 */

#define _GNU_SOURCE
#define TST_NO_DEFAULT_MAIN

#include <unistd.h>
#include "stdio.h"

#include "tst_test.h"
#include "tst_state_machine.h"

static const char *state_trace(struct tst_state_mach *mach)
{
	static char buf[4096];
	char *const *const names = mach->mat->names;
	size_t off = 1;
	unsigned c = 0, i;

	buf[0] = '\n';

	for (i = mach->top; c < 8; c++) {
		const struct tst_state_trace *t = mach->ring + i;

		if (!t->file)
			break;

		if (off >= sizeof(buf))
			break;

		off += snprintf(buf + off,
				sizeof(buf) - off - 1,
				"\t%s:%d %s (%u) -> %s (%u)\n",
				t->file, t->line,
				names[t->from], t->from,
				names[t->to], t->to);

		if (!i)
			i = 7;
		else
			i--;
	}

	return buf;
}

static void state_trace_set(const char *const file, const int lineno,
			    struct tst_state_trace *trace, unsigned from, unsigned to)
{
	trace->file = file;
	trace->line = lineno;
	trace->from = from;
	trace->to = to;
}

void tst_state_set(const char *const file, const int lineno,
		   struct tst_state_mach *mach, unsigned to)
{
	char *const *const names = mach->mat->names;
	const unsigned cur = mach->ring[mach->top].to;

	if (cur > 63)
		tst_brk_(file, lineno, TBROK, "Attempting to transition from an invalid state: %u: %s", cur, state_trace(mach));

	if (to > 63)
		tst_brk_(file, lineno, TBROK, "Attempting to transition to invalid state: %u: %s", to, state_trace(mach));

	if (!(mach->mat->states[cur] & (1 << to)))
		tst_brk_(file, lineno, TBROK, "Invalid transition: %s (%u) -> %s (%u): %s", names[cur], cur, names[to], to, state_trace(mach));

	if (++(mach->top) == 8)
		mach->top = 0;

	state_trace_set(file, lineno, &mach->ring[mach->top], cur, to);
}

unsigned tst_state_get(const char *const file, const int lineno,
		       struct tst_state_mach *mach, uint64_t mask)
{
	char *const *const names = mach->mat->names;
	const unsigned cur = mach->ring[mach->top].to;

	if (mask & (1 << cur))
		return cur;

	tst_brk_(file, lineno, TBROK, "Should not reach here while in state: %s (%u): %s",
		 names[cur], cur, state_trace(mach));

	return cur;
}

void tst_state_exp(const char *const file, const int lineno,
		   struct tst_state_mach *mach, uint64_t mask)
{
	tst_state_get(file, lineno, mach, mask);
}
