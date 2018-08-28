/*
 * Copyright (c) 2017 Richard Palethorpe <rpalethorpe@suse.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
/**
 * @file tst_fuzzy_sync.h
 * Fuzzy Synchronisation - abreviated to fzsync
 *
 * This library is intended to help reproduce race conditions by synchronising
 * two threads at a given place by marking the range a race may occur
 * in. Because the exact place where any race occurs is whithin the kernel,
 * and therefor impossible to mark accurately, the library may add randomised
 * delays to either thread in order to help find the exact race timing.
 *
 * Currently only two way races are explicitly supported, that is races
 * involving two threads or processes. We refer to the main test thread as
 * thread A and the child thread as thread B.
 *
 * In each thread you need a simple while or for loop which you call the
 * tst_fzsync_* functions in. In the simplest case thread A will look
 * something like:
 *
 * tst_fzsync_pair_reset(&pair, run_thread_b);
 * for (;;) {
 * 	// Perform some setup which must happen before the race
 * 	if (!tst_fzsync_start_race_a(&pair))
 * 		break;
 * 	// Do some dodgy syscall
 * 	tst_fzsync_end_race_a(&pair);
 * }
 *
 * Then in thread B (run_thread_b):
 *
 * while (tst_fzsync_start_race_b(&pair)) {
 * 	// Do something which can race with the dodgy syscall in A
 * 	if (!tst_fzsync_end_race_b(&pair))
 * 		break;
 * }
 *
 * The calls to tst_fzsync_start/end_race act as barriers which niether thread
 * will cross until the other has reached it. You can add more barriers by
 * using tst_fzsync_wait_a() and tst_fzsync_wait_b() in each thread.
 *
 * You may limit the loop in thread A to a certain number of loops or just
 * allow fzsync to timeout. This is 60 seconds by default, but you can change
 * it by setting tst_fzsync_pair::execution_time before calling
 * tst_fzsync_pair_reset().
 *
 * Generally speaking whatever loop or time limit you choose it will be wrong
 * on some subset of systems supported by Linux, but a best guess, based on
 * whatever systems you have access to, should suffice.
 *
 * It is possible to use the library just for tst_fzsync_pair_wait() to get a
 * basic spin wait. However if you are actually testing a race condition then
 * it is recommended to use tst_fzsync_start_race_a/b even if the
 * randomisation is not needed. It provides some semantic information which
 * may be useful in the future.
 *
 * For a usage example see testcases/cve/cve-2016-7117.c or just run
 * 'git grep tst_fuzzy_sync.h'
 *
 * @sa tst_fzsync_pair
 */

#include <sys/time.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include "tst_atomic.h"
#include "tst_timer.h"
#include "tst_safe_pthread.h"

/** Some statistics for a variable */
struct tst_fzsync_stat {
	float avg;
	float avg_dev;
	float dev_ratio;
};

/**
 * The state of a two way synchronisation or race.
 *
 * This contains all the necessary state for approximately synchronising two
 * sections of code in different threads.
 *
 * Internal fields should only be accessed by library functions.
 */
struct tst_fzsync_pair {
	/**
	 * The rate at which old diff samples are forgotten
	 *
	 * Defaults to 0.25.
	 */
	float avg_alpha;
	/** Internal; Thread A start time */
	struct timespec a_start;
	/** Internal; Thread B start time */
	struct timespec b_start;
	/** Internal; Thread A end time */
	struct timespec a_end;
	/** Internal; Thread B end time */
	struct timespec b_end;
	/** Internal; Avg. difference between a_start and b_start */
	struct tst_fzsync_stat diff_ss;
	/** Internal; Avg. difference between a_start and a_end */
	struct tst_fzsync_stat diff_sa;
	/** Internal; Avg. difference between b_start and b_end */
	struct tst_fzsync_stat diff_sb;
	/** Internal; Avg. difference between a_end and b_end */
	struct tst_fzsync_stat diff_ab;
	/** Internal; Number of spins while waiting for the slower thread */
	int spins;
	struct tst_fzsync_stat spins_avg;
	/**
	 * Internal; Number of spins to use in the delay.
	 *
	 * A negative value delays thread A and a positive delays thread B.
	 */
	int delay;
	/**
	 *  Internal; The number of samples left or the sampling state.
	 *
	 *  A positive value is the number of remaining mandatory
	 *  samples. Zero or a negative indicate ssome other state.
	 */
	int sampling;
	/**
	 * The Minimum number of statistical samples which must be collected.
	 *
	 * The minimum number of iterations which must be performed before a
	 * random delay can be calculated. Defaults to 1024.
	 */
	int min_samples;
	/**
	 * The maximum allowed proportional average deviation.
	 *
	 * A value in the range (0, 1) which gives the maximum average
	 * deviation which must be attained before random delays can be
	 * calculated.
	 *
	 * It is a ratio of (average_deviation / total_time). The default is
	 * 0.1, so this allows an average deviation of at most 10%.
	 */
	float max_dev_ratio;
        /** Internal; Atomic counter used by fzsync_pair_wait() */
	int a_cntr;
        /** Internal; Atomic counter used by fzsync_pair_wait() */
	int b_cntr;
	/** Internal; Used by tst_fzsync_pair_exit() and fzsync_pair_wait() */
	int exit;
	/** Internal; Used to limit the execution time. */
	struct tst_timer timer;
	/**
	 * The maximum time, in seconds, the test loop should be run.
	 *
	 * If the test runs for this amount of time without crashing or
	 * reaching some iteration limit, the wait and race functions will
	 * return zero signalling that the test loop should end.
	 *
	 * Note that this value is multiplied by LTP_TIMEOUT_MUL.
	 *
	 * Defaults to 60 seconds.
	 */
	int execution_time;
	/** Internal; The second thread or NULL */
	pthread_t thread_b;
};

/**
 * Default static values for struct tst_fzysnc_pair
 */
#define TST_FZSYNC_PAIR_INIT {	\
	.avg_alpha = 0.25,	\
	.min_samples = 1024,	\
	.max_dev_ratio = 0.1,	\
	.execution_time = 60	\
}

/**
 * Indicate that all threads should exit
 *
 * @relates tst_fzsync_pair
 *
 * Causes tst_fzsync_pair_wait(), and any functions which use it, to return 0.
 */
static inline void tst_fzsync_pair_exit(struct tst_fzsync_pair *pair)
{
	tst_atomic_store(1, &pair->exit);
}

/**
 * Exit and join thread B if necessary.
 *
 * @relates tst_fzsync_pair
 *
 * Call this from your cleanup function.
 */
static void tst_fzsync_pair_cleanup(struct tst_fzsync_pair *pair)
{
	if (pair->thread_b) {
		tst_fzsync_pair_exit(pair);
		SAFE_PTHREAD_JOIN(pair->thread_b, 0);
		pair->thread_b = 0;
	}
}

/**
 * Zero some stat fields
 *
 * @relates tst_fzsync_stat
 */
static void tst_init_stat(struct tst_fzsync_stat *s)
{
	s->avg = 0;
	s->avg_dev = 0;
}

/**
 * Reset or initialise fzsync.
 *
 * @relates tst_fzsync_pair
 * @param pair The state structure initialised with TST_FZSYNC_PAIR_INIT.
 * @param run_b The function defining thread B or NULL.
 *
 * Call this from your main test function (thread A), just before entering the
 * main loop. It will setup any values needed by fzsync and (re)start thread B
 * using the function provided.
 *
 * If you need to use fork or clone to start the second thread/process then
 * you can pass NULL to run_b and handle starting and stopping thread B
 * yourself. You may need to place tst_fzsync_pair in some shared memory as
 * well.
 */
static void tst_fzsync_pair_reset(struct tst_fzsync_pair *pair,
				  void *(*run_b)(void *))
{
	tst_fzsync_pair_cleanup(pair);

	tst_init_stat(&pair->diff_ss);
	tst_init_stat(&pair->diff_sa);
	tst_init_stat(&pair->diff_sb);
	tst_init_stat(&pair->diff_ab);
	tst_init_stat(&pair->spins_avg);
	pair->delay = 0;
	pair->sampling = pair->min_samples;

	pair->timer.limit =
		tst_sec_to_timespec(pair->execution_time * tst_timeout_mul());

	pair->a_cntr = 0;
	pair->b_cntr = 0;
	tst_atomic_store(0, &pair->exit);
	if (run_b)
		SAFE_PTHREAD_CREATE(&pair->thread_b, 0, run_b, 0);

	tst_timer_start_st(&pair->timer);
}

/**
 * Print stat
 *
 * @relates tst_fzsync_stat
 */
static inline void tst_fzsync_stat_info(struct tst_fzsync_stat stat,
					char *unit, char *name)
{
	tst_res(TINFO,
		"%1$-17s: { avg = %3$5.0f%2$s, avg_dev = %4$5.0f%2$s, dev_ratio = %5$.2f }",
		name, unit, stat.avg, stat.avg_dev, stat.dev_ratio);
}

/**
 * Print some synchronisation statistics
 *
 * @relates tst_fzsync_pair
 */
static void tst_fzsync_pair_info(struct tst_fzsync_pair *pair, int loop_index)
{
	tst_res(TINFO, "loop = %d", loop_index);
	tst_fzsync_stat_info(pair->diff_ss, "ns", "start_a - start_b");
	tst_fzsync_stat_info(pair->diff_sa, "ns", "end_a - start_a");
	tst_fzsync_stat_info(pair->diff_sb, "ns", "end_b - start_b");
	tst_fzsync_stat_info(pair->diff_ab, "ns", "end_a - end_b");
	tst_fzsync_stat_info(pair->spins_avg, "  ", "spins");
}

/** Wraps clock_gettime */
static inline void tst_fzsync_time(struct timespec *t)
{
	clock_gettime(CLOCK_MONOTONIC_RAW, t);
}

/**
 * Exponential moving average
 *
 * @param alpha The preference for recent samples over old ones.
 * @param sample The current sample
 * @param prev_avg The average of the all the previous samples
 *
 * @return The average including the current sample.
 */
static inline float tst_exp_moving_avg(float alpha,
					float sample,
					float prev_avg)
{
	return alpha * sample + (1.0 - alpha) * prev_avg;
}

/**
 * Update a stat with a new sample
 *
 * @relates tst_fzsync_stat
 */
static inline void tst_upd_stat(struct tst_fzsync_stat *s,
				 float alpha,
				 float sample)
{
	s->avg = tst_exp_moving_avg(alpha, sample, s->avg);
	s->avg_dev = tst_exp_moving_avg(alpha, fabs(s->avg - sample), s->avg_dev);
	s->dev_ratio = fabs(s->avg ? s->avg_dev / s->avg : 0);
}

/**
 * Update a stat with a new diff sample
 *
 * @relates tst_fzsync_stat
 */
static inline void tst_upd_diff_stat(struct tst_fzsync_stat *s,
				     float alpha,
				     struct timespec t1,
				     struct timespec t2)
{
	tst_upd_stat(s, alpha, tst_timespec_diff_ns(t1, t2));
}

/**
 * Calculate various statistics and the delay
 *
 * This function helps create the fuzz in fuzzy sync. Imagine we have the
 * following timelines in threads A and B:
 *
 *  start_race_a
 *      ^                    end_race_a (a)
 *      |                        ^
 *      |                        |
 *  - --+------------------------+-- - -
 *      |        Syscall A       |                 Thread A
 *  - --+------------------------+-- - -
 *  - --+----------------+-------+-- - -
 *      |   Syscall B    | spin  |                 Thread B
 *  - --+----------------+-------+-- - -
 *      |                |
 *      ^                ^
 *  start_race_b     end_race_b
 *
 * Here we have synchronised the calls to syscall A and B with start_race_{a,
 * b} so that they happen at approximately the same time in threads A and
 * B. If the race condition occurs during the entry code for these two
 * functions then we will quickly hit it. If it occurs during the exit code of
 * B and mid way through A, then we will quickly hit it.
 *
 * However if the exit paths of A and B need to be aligned and (end_race_a -
 * end_race_b) is large relative to the variation in call times, the
 * probability of hitting the race condition is close to zero. To solve this
 * scenario (and others) a randomised delay is introduced before the syscalls
 * in A and B. Given enough time the following should happen where the exit
 * paths are now synchronised:
 *
 *  start_race_a
 *      ^                    end_race_a (a)
 *      |                        ^
 *      |                        |
 *  - --+------------------------+-- - -
 *      |        Syscall A       |                 Thread A
 *  - --+------------------------+-- - -
 *  - --+-------+----------------+-- - -
 *      | delay |   Syscall B    |                 Thread B
 *  - --+-------+----------------+-- - -
 *      |                        |
 *      ^                        ^
 *  start_race_b             end_race_b
 *
 * The delay is not introduced immediately and the delay range is only
 * calculated once the average relative deviation has dropped below some
 * percentage of the total time.
 *
 * The delay range is choosen so that any point in Syscall A could be
 * synchronised with any point in Syscall B using a value from the
 * range. Because the delay range may be too large for a linear search, we use
 * an evenly distributed random function to pick a value from it.
 *
 * @relates tst_fzsync_pair
 */
static void tst_fzsync_pair_update(int loop_index, struct tst_fzsync_pair *pair)
{
	float alpha = pair->avg_alpha;
	float per_spin_time, time_delay, dev_ratio;

	dev_ratio = (pair->diff_sa.dev_ratio
		     + pair->diff_sb.dev_ratio
		     + pair->diff_ab.dev_ratio
		     + pair->spins_avg.dev_ratio) / 4;

	if (pair->sampling > 0 || dev_ratio > pair->max_dev_ratio) {
		tst_upd_diff_stat(&pair->diff_ss, alpha,
				  pair->a_start, pair->b_start);
		tst_upd_diff_stat(&pair->diff_sa, alpha,
				  pair->a_end, pair->a_start);
		tst_upd_diff_stat(&pair->diff_sb, alpha,
				  pair->b_end, pair->b_start);
		tst_upd_diff_stat(&pair->diff_ab, alpha,
				  pair->a_end, pair->b_end);
		tst_upd_stat(&pair->spins_avg, alpha, pair->spins);
		if (pair->sampling > 0 && --pair->sampling == 0) {
			tst_res(TINFO,
				"Minimum sampling period ended, deviation ratio = %.2f",
				dev_ratio);
			tst_fzsync_pair_info(pair, loop_index);
		}
	} else if (pair->diff_ab.avg >= 1 && pair->spins_avg.avg >= 1) {
		per_spin_time = fabs(pair->diff_ab.avg) / pair->spins_avg.avg;
		time_delay = drand48() * (pair->diff_sa.avg + pair->diff_sb.avg)
			- pair->diff_sb.avg;
		pair->delay = (int)(time_delay / per_spin_time);

		if (!pair->sampling) {
			tst_res(TINFO,
				"Reached deviation ratio %.2f (max %.2f), introducing randomness",
				dev_ratio, pair->max_dev_ratio);
			tst_res(TINFO, "Delay range is [-%d, %d]",
				(int)(pair->diff_sb.avg / per_spin_time),
				(int)(pair->diff_sa.avg / per_spin_time));
			tst_fzsync_pair_info(pair, loop_index);
			pair->sampling = -1;
		}
	} else if (!pair->sampling) {
		tst_res(TWARN, "Can't calculate random delay");
		pair->sampling = -1;
	}

	pair->spins = 0;
}

/**
 * Wait for the other thread
 *
 * @relates tst_fzsync_pair
 * @param our_cntr The counter for the thread we are on
 * @param other_cntr The counter for the thread we are synchronising with
 * @param spins A pointer to the spin counter or NULL
 *
 * Used by tst_fzsync_pair_wait_a(), tst_fzsync_pair_wait_b(),
 * tst_fzsync_start_race_a(), etc. If the calling thread is ahead of the other
 * thread, then it will spin wait. Unlike pthread_barrier_wait it will never
 * use futex and can count the number of spins spent waiting.
 *
 * @return A non-zero value if the thread should continue otherwise the
 * calling thread should exit.
 */
static inline int tst_fzsync_pair_wait(struct tst_fzsync_pair *pair,
				       int *our_cntr,
				       int *other_cntr,
				       int *spins)
{
	if (tst_atomic_inc(other_cntr) == INT_MAX) {
		/*
		 * We are about to break the invariant that the thread with
		 * the lowest count is in front of the other. So we must wait
		 * here to ensure the other thread has atleast reached the
		 * line above before doing that. If we are in rear position
		 * then our counter may already have been set to zero.
		 */
		while (tst_atomic_load(our_cntr) > 0
		       && tst_atomic_load(our_cntr) < INT_MAX
		       && !tst_atomic_load(&pair->exit)) {
			if (spins)
				(*spins)++;
		}

		tst_atomic_store(0, other_cntr);
		/*
		 * Once both counters have been set to zero the invariant
		 * is restored and we can continue.
		 */
		while (tst_atomic_load(our_cntr) > 1
		       && !tst_atomic_load(&pair->exit))
			;
	} else {
		/*
		 * If our counter is less than the other thread's we are ahead
		 * of it and need to wait.
		 */
		while (tst_atomic_load(our_cntr) < tst_atomic_load(other_cntr)
		       && !tst_atomic_load(&pair->exit)) {
			if (spins)
				(*spins)++;
		}
	}

	return !tst_atomic_load(&pair->exit);
}

/**
 * Wait in thread A
 *
 * @relates tst_fzsync_pair
 * @sa tst_fzsync_pair_wait
 */
static inline int tst_fzsync_wait_a(struct tst_fzsync_pair *pair)
{
	return tst_fzsync_pair_wait(pair, &pair->a_cntr, &pair->b_cntr, NULL);
}

/**
 * Wait in thread B
 *
 * @relates tst_fzsync_pair
 * @sa tst_fzsync_pair_wait
 */
static inline int tst_fzsync_wait_b(struct tst_fzsync_pair *pair)
{
	return tst_fzsync_pair_wait(pair, &pair->b_cntr, &pair->a_cntr, NULL);
}

/**
 * Marks the start of a race region in thread A
 *
 * @relates tst_fzsync_pair
 *
 * This should be placed just before performing whatever action can cause a
 * race condition. Usually it is placed just before a syscall and
 * tst_fzsync_end_race_a() is placed just afterward.
 *
 * A corrosponding call to tst_fzsync_start_race_b() should be made in thread
 * B.
 *
 * @return A non-zero value if the calling thread should continue to loop. If
 * it returns zero then tst_fzsync_exit() has been called and you must exit
 * the thread.
 *
 * @sa tst_fzsync_pair_update
 */
static inline int tst_fzsync_start_race_a(struct tst_fzsync_pair *pair)
{
	static int loop_index;
	volatile int delay;
	int ret;

	tst_fzsync_wait_a(pair);

	if (tst_timer_expired_st(&pair->timer)) {
		tst_res(TINFO,
			"Exceeded fuzzy sync time limit, requesting exit");
		tst_fzsync_pair_exit(pair);
	} else {
		loop_index++;
		tst_fzsync_pair_update(loop_index, pair);
	}

	ret = tst_fzsync_wait_a(pair);
	tst_fzsync_time(&pair->a_start);

	delay = pair->delay;
	while (delay < 0)
		delay++;

	return ret;
}

/**
 * Marks the end of a race region in thread A
 *
 * @relates tst_fzsync_pair
 * @sa tst_fzsync_start_race_a
 */
static inline int tst_fzsync_end_race_a(struct tst_fzsync_pair *pair)
{
	tst_fzsync_time(&pair->a_end);
	return tst_fzsync_pair_wait(pair, &pair->a_cntr, &pair->b_cntr,
				    &pair->spins);
}

/**
 * Marks the start of a race region in thread B
 *
 * @relates tst_fzsync_pair
 * @sa tst_fzsync_start_race_a
 */
static inline int tst_fzsync_start_race_b(struct tst_fzsync_pair *pair)
{
	int ret;
	volatile int delay;

	tst_fzsync_wait_b(pair);
	ret = tst_fzsync_wait_b(pair);
	tst_fzsync_time(&pair->b_start);

	delay = pair->delay;
	while (delay > 0)
		delay--;

	return ret;
}

/**
 * Marks the end of a race region in thread B
 *
 * @relates tst_fzsync_pair
 * @sa tst_fzsync_start_race_a
 */
static inline int tst_fzsync_end_race_b(struct tst_fzsync_pair *pair)
{
	tst_fzsync_time(&pair->b_end);
	return tst_fzsync_pair_wait(pair, &pair->b_cntr, &pair->a_cntr,
				    &pair->spins);
}
