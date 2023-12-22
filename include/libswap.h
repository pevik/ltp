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

/*
 * Make a swap file
 */
int make_swapfile(const char *swapfile, int safe);

/*
 * Check swapon/swapoff support status of filesystems or files
 * we are testing on.
 */
void is_swap_supported(const char *filename);

/*
 * Get kernel constant MAX_SWAPFILES value
 */
unsigned int get_maxswapfiles(void);

/*
 * Get the used swapfiles number
 */
int get_used_swapfiles(const char *file, const int lineno);
#define GET_USED_SWAPFILES() \
	get_used_swapfiles(__FILE__, __LINE__)

#endif /* __LIBSWAP_H__ */
