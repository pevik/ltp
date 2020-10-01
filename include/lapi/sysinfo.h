// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2020 Petr Vorel <petr.vorel@gmail.com>
 */

#ifndef SYSINFO_H__

/*
 * Don't use <sys/sysinfo.h> as it breaks build MUSL toolchain.
 * Use <linux/sysinfo.h> instead.
 *
 * Some kernel UAPI headers do indirect <linux/sysinfo.h> include:
 * <linux/netlink.h> or others -> <linux/kernel.h> -> <linux/sysinfo.h>
 *
 * This indirect include causes on MUSL redefinition of struct sysinfo when
 * included both <sys/sysinfo.h> and some of UAPI headers:
 */
#include <linux/sysinfo.h>

#define SYSINFO_H__

#endif /* SYSINFO_H__ */
