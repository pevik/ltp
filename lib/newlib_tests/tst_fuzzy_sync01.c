// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2021 Richard Palethorpe <rpalethorpe@suse.com>
 */
#define _GNU_SOURCE

#include "tst_test.h"
#include "tst_fuzzy_sync.h"

enum race_window_state {
	/* We are in a race window which will cause behaviour that has
	 * very different timings to the target. We must avoid these
	 * race windows. */
	TOO_EARLY,
	TOO_LATE,
	/* We are not in the target race window, but the timings of
	 * this behaviour are similar to those of the target. */
	EARLY_OR_LATE,
	/* We in the target race window */
	CRITICAL,
};

enum wait_with {
	SPIN,
	SLEEP,
};

struct window {
	const enum wait_with type;
	const int time;
};

struct race_shape {
	const struct window too_early;
	const struct window early;
	const struct window critical;
	const struct window late;
	const struct window too_late;
};

struct scenario {
	const char *const name;
	const struct race_shape a;
	const struct race_shape b;
};

static struct tst_fzsync_pair pair;

static volatile enum race_window_state state;

static const struct scenario races[] = {
	{ "No delay",
	  {{SPIN, 0 }, {SPIN, 0 }, {SPIN, 100 }, {SPIN, 0 }, {SPIN, 0 }},
	  {{SPIN, 0 }, {SPIN, 0 }, {SPIN, 100 }, {SPIN, 0 }, {SPIN, 0 }},
	},
	{ "Exit aligned",
	  {{SPIN, 0 }, {SLEEP, 1 }, {SPIN, 10 }, {SPIN, 10 }, {SPIN, 0 }},
	  {{SPIN, 0 }, {SLEEP, 1 }, {SPIN, 1000 }, {SPIN, 10 }, {SPIN, 0 }},
	},
	{ "Entry aligned",
	  {{SPIN, 0 }, {SPIN, 1 }, {SPIN, 10 }, {SLEEP, 10 }, {SPIN, 0 }},
	  {{SPIN, 0 }, {SPIN, 1 }, {SPIN, 1000 }, {SLEEP, 1 }, {SPIN, 0 }},
	},
	{ "delay A entry",
	  {{SPIN, 0 }, {SLEEP, 1 }, {SPIN, 10 }, {SPIN, 10 }, {SPIN, 0 }},
	  {{SPIN, 0 }, {SPIN, 1 }, {SPIN, 1000 }, {SPIN, 1 }, {SPIN, 0 }},
	},
	{ "delay B entry",
	  {{SPIN, 0 }, {SPIN, 1 }, {SPIN, 10 }, {SPIN, 10 }, {SPIN, 0 }},
	  {{SPIN, 0 }, {SLEEP, 1 }, {SPIN, 1000 }, {SPIN, 1 }, {SPIN, 0 }},
	},
	{ "delay A exit",
	  {{SPIN, 0 }, {SPIN, 1 }, {SPIN, 10 }, {SLEEP, 10 }, {SPIN, 0 }},
	  {{SPIN, 0 }, {SPIN, 1 }, {SPIN, 1000 }, {SPIN, 1 }, {SPIN, 0 }},
	},
	{ "delay B exit",
	  {{SPIN, 0 }, {SPIN, 1 }, {SPIN, 10 }, {SPIN, 10 }, {SPIN, 0 }},
	  {{SPIN, 0 }, {SPIN, 1 }, {SPIN, 1000 }, {SLEEP, 1 }, {SPIN, 0 }},
	},
	{ "too early",
	  {{SPIN, 10 }, {SLEEP, 1 }, {SPIN, 100 }, {SPIN, 100 }, {SPIN, 0 }},
	  {{SLEEP, 1 }, {SPIN, 1000 }, {SPIN, 1000 }, {SPIN, 10 }, {SPIN, 0 }},
	},
	{ "too late",
	  {{SPIN, 10000 }, {SLEEP, 1 }, {SPIN, 100 }, {SPIN, 100 }, {SPIN, 10 }},
	  {{SPIN, 0 }, {SPIN, 1000 }, {SPIN, 1000 }, {SPIN, 1000 }, {SPIN, 10 }},
	},
};

static void cleanup(void)
{
	tst_fzsync_pair_cleanup(&pair);
}

static void setup(void)
{
	pair.min_samples = 10000;

	tst_fzsync_pair_init(&pair);
}

static void delay(struct window w)
{
	struct timespec ts = { 0, w.time };
	volatile int time = w.time;

	if (pair.yield_in_wait)
		time /= 100;

	switch(w.type) {
	case SPIN:
		while (time--) {
			if (pair.yield_in_wait)
				sched_yield();
		}
		break;
	case SLEEP:
		nanosleep(&ts, NULL);
		break;
	}
}

static void *worker(void *v)
{
	unsigned int i = *(unsigned int *)v;
	const struct race_shape *r = &races[i].b;

	while (tst_fzsync_run_b(&pair)) {
		state = EARLY_OR_LATE;

		tst_fzsync_start_race_b(&pair);
		if (r->too_early.time) {
			state = TOO_EARLY;
			delay(r->too_early);
		}

		state = EARLY_OR_LATE;
		delay(r->early);

		state = CRITICAL;
		delay(r->critical);

		state = EARLY_OR_LATE;
		delay(r->late);

		if (r->too_late.time) {
			state = TOO_LATE;
			delay(r->too_late);
		}
		tst_fzsync_end_race_b(&pair);
	}

	return NULL;
}

static void run(unsigned int i)
{
	const struct race_shape *r = &races[i].a;
	struct tst_fzsync_run_thread wrap_run_b = {
		.func = worker,
		.arg = &i,
	};
	int too_early = 0, early_or_late = 0, critical = 0, too_late = 0;
	enum race_window_state state_copy;


	tst_fzsync_pair_reset(&pair, NULL);
	SAFE_PTHREAD_CREATE(&pair.thread_b, 0, tst_fzsync_thread_wrapper,
			    &wrap_run_b);

	while (tst_fzsync_run_a(&pair)) {
		tst_fzsync_start_race_a(&pair);

		delay(r->too_early);
		state_copy = state;

		switch(state_copy) {
		case TOO_EARLY:
		case TOO_LATE:
			break;
		default:
			delay(r->early);
			state_copy = state;
		}

		switch(state_copy) {
		case TOO_EARLY:
			too_early++;
			break;
		case EARLY_OR_LATE:
			early_or_late++;
			delay(r->late);
			break;
		case CRITICAL:
			critical++;
			delay(r->critical);
			break;
		case TOO_LATE:
			too_late++;
			delay(r->too_late);
			break;
		default:
			tst_brk(TBROK, "enum is not atomic?");
		}
		tst_fzsync_end_race_a(&pair);

		switch(state_copy) {
		case TOO_EARLY:
			tst_fzsync_pair_add_bias(&pair, -10);
			break;
		case TOO_LATE:
			tst_fzsync_pair_add_bias(&pair, 10);
			break;
		default:
			;
		}

		if (critical > 100) {
			tst_fzsync_pair_cleanup(&pair);
			break;
		}
	}

	tst_res(critical ? TPASS : TFAIL, "%20s) =%-4d ~%-4d -%-4d +%-4d",
		races[i].name, critical, early_or_late, too_early, too_late);
}

static struct tst_test test = {
	.tcnt = ARRAY_SIZE(races),
	.test = run,
	.setup = setup,
	.cleanup = cleanup,
};
