// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2022 SUSE LLC <rpalethorpe@suse.com>
 */

#include "inttypes.h"

#ifndef TST_STATE_MACHINE_H
#define TST_STATE_MACHINE_H

#define TST_STATE_ANY (~(uint64_t)0)

struct tst_state_matrix {
	char *names[64];
	uint64_t states[64];
};

struct tst_state_trace {
	const char *file;
	int line;
	unsigned from;
	unsigned to;
};

struct tst_state_mach {
	const struct tst_state_matrix *mat;

	unsigned top;
	struct tst_state_trace ring[8];
};

#define TST_STATE_SET(mach, to) \
	tst_state_set(__FILE__, __LINE__, mach, to)

void tst_state_set(const char *const file, const int lineno,
		  struct tst_state_mach *mach, unsigned to);

#define TST_STATE_EXP(mach, mask) \
	tst_state_exp(__FILE__, __LINE__, mach, mask)

void tst_state_exp(const char *const file, const int lineno,
		   struct tst_state_mach *mach, uint64_t mask);

#define TST_STATE_GET(mach, mask) \
	tst_state_get(__FILE__, __LINE__, mach, mask)

unsigned tst_state_get(const char *const file, const int lineno,
		       struct tst_state_mach *mach, uint64_t mask);

#endif
