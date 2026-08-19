// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2026 Cyril Hrubis <chrubis@suse.cz>
 */

#ifndef SYS_NET_COMMON
#define SYS_NET_COMMON

struct netdev_state {
	long carrier;
	long carrier_changes;
	long carrier_up_count;
	long carrier_down_count;
	char operstate[32];
};

static void read_state(struct netdev_state *st)
{
	st->carrier = TST_SYSFS_READ_LI(IFACE_PATH "/carrier");
	st->carrier_changes = TST_SYSFS_READ_LI(IFACE_PATH "/carrier_changes");
	st->carrier_up_count = TST_SYSFS_READ_LI(IFACE_PATH "/carrier_up_count");
	st->carrier_down_count = TST_SYSFS_READ_LI(IFACE_PATH "/carrier_down_count");
	TST_SYSFS_READ_STR(st->operstate, sizeof(st->operstate), IFACE_PATH "/operstate");
}

static void check_state(const struct netdev_state *st, long carrier,
			const char *operstate, const char *desc)
{
	tst_res(TINFO, "%s", desc);

	TST_EXP_EQ_LI(st->carrier, carrier);
	TST_EXP_EQ_STR(st->operstate, operstate);
	TST_EXP_EQ_LI(st->carrier_changes,
		      st->carrier_up_count + st->carrier_down_count);
}

static void check_state_delta(const struct netdev_state *prev,
			      const struct netdev_state *cur, long up_delta,
			      long down_delta, const char *desc)
{
	long got_up = cur->carrier_up_count - prev->carrier_up_count;
	long got_down = cur->carrier_down_count - prev->carrier_down_count;
	long got_changes = cur->carrier_changes - prev->carrier_changes;

	tst_res(TINFO, "%s", desc);

	TST_EXP_EQ_LI(got_up, up_delta);
	TST_EXP_EQ_LI(got_down, down_delta);
	TST_EXP_EQ_LI(got_changes, up_delta + down_delta);
}

static void check_mtu_valid(long mtu)
{
	char mtu_str[16];

	snprintf(mtu_str, sizeof(mtu_str), "%ld", mtu);

	if (FILE_PRINTF(IFACE_PATH "/mtu", "%s", mtu_str)) {
		tst_res(TFAIL, "Failed to set mtu to %ld", mtu);
		return;
	}

	TST_EXP_EQ_LI(read_mtu(), mtu);
}

static void check_mtu_invalid(const char *mtu_str)
{
	long before = read_mtu();

	if (!FILE_PRINTF(IFACE_PATH "/mtu", "%s", mtu_str)) {
		tst_res(TFAIL, "Writing mtu '%s' unexpectedly succeeded",
			mtu_str);
		return;
	}

	tst_res(TINFO, "Writing mtu '%s' was rejected as expected", mtu_str);
	TST_EXP_EQ_LI(read_mtu(), before);
}

#endif /* SYS_NET_COMMON */
