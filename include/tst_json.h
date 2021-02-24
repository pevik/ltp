// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2021 Cyril Hrubis <metan@ucw.cz>
 */

#ifndef TST_JSON_H
#define TST_JSON_H

#include <stdio.h>

#define TST_JSON_ERR_MAX 128
#define TST_JSON_ID_MAX 64

enum tst_json_type {
	TST_JSON_VOID = 0,
	TST_JSON_INT,
	TST_JSON_STR,
	TST_JSON_OBJ,
	TST_JSON_ARR,
};

struct tst_json_buf {
	/** Pointer to a null terminated JSON string */
	const char *json;
	/** A length of the JSON string */
	size_t len;
	/** A current offset into the JSON string */
	size_t off;
	/** An offset to the start of the last array or object */
	size_t sub_off;

	char err[TST_JSON_ERR_MAX];
	char buf[];
};

struct tst_json_val {
	enum tst_json_type type;

	/** An user supplied buffer and size to store a string values to. */
	char *buf;
	size_t buf_size;

	/** An union to store the parsed value into. */
	union {
		long val_int;
		const char *val_str;
	};

	/** An ID for object values */
	char id[TST_JSON_ID_MAX];
};

/*
 * @brief Resets the parser.
 *
 * Resets the parse offset and clears errors.
 *
 * @buf An tst_json buffer
 */
static inline void tst_json_reset(struct tst_json_buf *buf)
{
	buf->off = 0;
	buf->err[0] = 0;
}

/*
 * @brief Fills the buffer error.
 *
 * Once buffer error is set all parsing functions return immediatelly with type
 * set to TST_JSON_VOID.
 *
 * @buf An tst_json buffer
 * @fmt A printf like format string
 * @... A printf like parameters
 */
void tst_json_err(struct tst_json_buf *buf, const char *fmt, ...)
               __attribute__((format (printf, 2, 3)));

/*
 * @brief Prints error into a file.
 *
 * The error takes into consideration the current offset in the buffer and
 * prints a few preceding lines along with the exact position of the error.
 *
 * @f A file to print the error to.
 * @buf An tst_json buffer.
 */
void tst_json_err_print(FILE *f, struct tst_json_buf *buf);

/*
 * @brief Returns true if error was encountered.
 *
 * @bfu An tst_json buffer.
 * @return True if error was encountered false otherwise.
 */
static inline int tst_json_is_err(struct tst_json_buf *buf)
{
	return !!buf->err[0];
}

/*
 * @brief Checks is result has valid type.
 *
 * @res An tst_json value.
 * @return Zero if result is not valid, non-zero otherwise.
 */
static inline int tst_json_valid(struct tst_json_val *res)
{
	return !!res->type;
}

/*
 * @brief Returns the type of next element in buffer.
 *
 * @buf An tst_json buffer.
 * @return A type of next element in the buffer.
 */
enum tst_json_type tst_json_next_type(struct tst_json_buf *buf);

/*
 * @brief Returns if first element in JSON is object or array.
 *
 * @buf An tst_json buffer.
 * @return On success returns TST_JSON_OBJ or TST_JSON_ARR. On failure TST_JSON_VOID.
 */
enum tst_json_type tst_json_start(struct tst_json_buf *buf);

/*
 * @brief Starts parsing of an JSON object.
 *
 * @buf An tst_json buffer.
 * @res An tst_json result.
 */
int tst_json_obj_first(struct tst_json_buf *buf, struct tst_json_val *res);
int tst_json_obj_next(struct tst_json_buf *buf, struct tst_json_val *res);

#define TST_JSON_OBJ_FOREACH(buf, res) \
	for (tst_json_obj_first(buf, res); tst_json_valid(res); tst_json_obj_next(buf, res))

/*
 * @brief Skips parsing of an JSON object.
 *
 * @buf An tst_json buffer.
 * @return Zero on success, non-zero otherwise.
 */
int tst_json_obj_skip(struct tst_json_buf *buf);

int tst_json_arr_first(struct tst_json_buf *buf, struct tst_json_val *res);
int tst_json_arr_next(struct tst_json_buf *buf, struct tst_json_val *res);

#define TST_JSON_ARR_FOREACH(buf, res) \
	for (tst_json_arr_first(buf, res); tst_json_valid(res); tst_json_arr_next(buf, res))

/*
 * @brief Skips parsing of an JSON array.
 *
 * @buf An tst_json buffer.
 * @return Zero on success, non-zero otherwise.
 */
int tst_json_arr_skip(struct tst_json_buf *buf);

/*
 * @brief Loads a file into an tst_json buffer.
 *
 * @path A path to a file.
 * @return An tst_json buffer or NULL in a case of a failure.
 */
struct tst_json_buf *tst_json_load(const char *path);

/*
 * @brief Frees an tst_json buffer.
 *
 * @buf An tst_json buffer allcated by tst_json_load() function.
 */
void tst_json_free(struct tst_json_buf *buf);

#endif /* TST_JSON_H */
