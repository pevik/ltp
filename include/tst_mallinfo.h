// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2021 FUJITSU LIMITED. All rights reserved.
 * Author: Yang Xu <xuyang2018.jy@cn.fujitsu.com>
 */

#ifndef TST_MALLINFO_H__
#define TST_MALLINFO_H__

#include <malloc.h>
#include "config.h"

#ifdef HAVE_MALLINFO
void tst_print_mallinfo(const char *msg, struct mallinfo *m);
#endif

#endif
