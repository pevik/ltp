/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2015-2025 Cyril Hrubis <chrubis@suse.cz>
 * Copyright (c) Linux Test Project, 2017-2025
 */

#ifndef TST_INTERNAL_H__
#define TST_INTERNAL_H__

/*
 * Returns NULL-terminated array of kernel-supported filesystems.
 *
 * @skiplist A NULL terminated array of filesystems to skip.
 */
const char **tst_get_supported_fs_types(const char *const *skiplist);

#endif /* TST_INTERNAL_H__ */
