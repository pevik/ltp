// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2013 Oracle and/or its affiliates. All Rights Reserved.
 * Author: Stanislav Kholmanskikh <stanislav.kholmanskikh@oracle.com>
 */

/*
 * Contains common content for all swapon/swapoff tests
 */

#ifndef __LIBSWAP_H__
#define __LIBSWAP_H__

enum swapfile_method {
    SWAPFILE_BY_SIZE,
    SWAPFILE_BY_BLKS
};

/*
 * Create a swapfile of a specified size or number of blocks.
 */
int make_swapfile(const char *swapfile, unsigned int num,
			int safe, enum swapfile_method method);

#define MAKE_SWAPFILE_SIZE(swapfile, size, safe) \
    make_swapfile(swapfile, size, safe, SWAPFILE_BY_SIZE)

#define MAKE_SWAPFILE_BLKS(swapfile, blocks, safe) \
    make_swapfile(swapfile, blocks, safe, SWAPFILE_BY_BLKS)

/*
 * Check swapon/swapoff support status of filesystems or files
 * we are testing on.
 */
bool is_swap_supported(const char *filename);

/*
 * Get kernel constant MAX_SWAPFILES value.
 *
 */
int tst_max_swapfiles(void);

/*
 * Get the used swapfiles number.
 */
int tst_count_swaps(void);

#endif /* __LIBSWAP_H__ */
