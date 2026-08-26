// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2020 FUJITSU LIMITED. All rights reserved.
 * Author: Yang Xu <xuyang2018.jy@cn.fujitsu.com>
 * Copyright (c) 2020 Cyril Hrubis <chrubis@suse.cz>
 */
#ifndef TST_ASSERT_H__
#define TST_ASSERT_H__

#define TST_ASSERT_INT(path, val) \
	tst_assert_int(__FILE__, __LINE__, path, val)

/*
 * Asserts that integer value stored in file pointed by path equals to the
 * value passed to this function. This is mostly useful for asserting correct
 * values in sysfs, procfs, etc.
 */
void tst_assert_int(const char *file, const int lineno,
		    const char *path, int val);

#define TST_ASSERT_FILE_INT(path, prefix, val) \
	tst_assert_file_int(__FILE__, __LINE__, path, prefix, val)

/**
 * TST_ASSERT_SATURATED_INT - Clamp the sysfs/procfs value to INT_MAX.
 *
 * If this flag is set and the value read from the sysfs/procfs file exceeds
 * INT_MAX, the compared value is clamped (saturated) to INT_MAX.
 */
#define TST_ASSERT_SATURATED_INT   0x01

/**
 * TST_ASSERT_TRUNC_32BIT - Keep only the lower 32 bits of the read value.
 *
 * If this flag is set, only the low 32 bits of the value read from the
 * sysfs/procfs file are compared.
 */
#define TST_ASSERT_TRUNC_32BIT     0x02

/**
 * tst_assert_ulong() - Assert that an unsigned long value in a file matches.
 * @file: The source file of the assertion (usually __FILE__).
 * @lineno: The source line number of the assertion (usually __LINE__).
 * @path: Path to the sysfs or procfs file to read from.
 * @val: The expected unsigned long value to compare against.
 * @flags: Bitwise flags controlling how the read value is processed.
 *         Supported flags:
 *         - %TST_ASSERT_SATURATED_INT: Clamps the value at %INT_MAX if it
 *           exceeds it.
 *         - %TST_ASSERT_TRUNC_32BIT: Keeps only the low 32 bits of the
 *           read value, truncating any higher bits.
 *         - %0: Direct comparison without any adjustment.
 *
 * This function reads an integer value from the file specified by @path
 * and compares it with @val. It allows handling of 32-bit compat mode
 * truncation/clamping on 64-bit systems via @flags.
 */
void tst_assert_ulong(const char *file, const int lineno,
                      const char *path, unsigned long val, int flags);

#define TST_ASSERT_ULONG(path, val, ...) \
	tst_assert_ulong(__FILE__, __LINE__, path, val, \
		TST_2_(dummy, ##__VA_ARGS__, 0))

/*
 * Asserts that integer value stored in the prefix field of file pointed by path
 * equals to the value passed to this function. This is mostly useful for
 * asserting correct field values in sysfs, procfs, etc.
 */

void tst_assert_file_int(const char *file, const int lineno,
			 const char *path, const char *prefix, int val);


#define TST_ASSERT_STR(path, val) \
	tst_assert_str(__FILE__, __LINE__, path, val)

/*
 * Asserts that a string value stored in file pointed by path equals to the
 * value passed to this function. This is mostly useful for asserting correct
 * values in sysfs, procfs, etc.
 */
void tst_assert_str(const char *file, const int lineno,
		    const char *path, const char *val);

#define TST_ASSERT_FILE_STR(path, prefix, val) \
	tst_assert_file_str(__FILE__, __LINE__, path, prefix, val)

/*
 * Asserts that a string value stored in the prefix field of file pointed by path
 * equals to the value passed to this function. This is mostly useful for
 * asserting correct field values in sysfs, procfs, etc.
 */
void tst_assert_file_str(const char *file, const int lineno,
			 const char *path, const char *prefix, const char *val);

#endif /* TST_ASSERT_H__ */
