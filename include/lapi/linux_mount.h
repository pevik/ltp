// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) Linux Test Project, 2015-2025
 */

#ifndef LINUX_LAPI_MOUNT_H__
#define LINUX_LAPI_MOUNT_H__

/*
 * NOTE: <linux/mount.h> conflicts with <sys/mount.h>, therefore all fallback
 * definitions from both of them should go to lapi/mount.h.
 */
#ifdef HAVE_LINUX_MOUNT_H
# include <linux/mount.h>
#endif

#include "lapi/mount.h"

#endif /* LINUX_LAPI_MOUNT_H__ */
