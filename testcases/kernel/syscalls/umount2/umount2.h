// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2016 Cyril Hrubis <chrubis@suse.cz>
 */

#ifndef UMOUNT2_H__
#define UMOUNT2_H__

static inline int umount2_retry(const char *target, int flags)
{
	int i, ret;

	for (i = 0; i < 50; i++) {
		ret = umount2(target, flags);

		if (ret == 0 || errno != EBUSY)
			return ret;

		tst_res(TINFO, "umount('%s', %i) failed with EBUSY, try %2i...",
			 target, flags, i);

		usleep(100000);
	}

	tst_res(TWARN, "Failed to umount('%s', %i) after 50 retries",
	         target, flags);

	errno = EBUSY;
	return -1;
}

#endif	/* UMOUNT2_H__ */
