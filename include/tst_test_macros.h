// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2015-2024 Cyril Hrubis <chrubis@suse.cz>
 * Copyright (c) Linux Test Project, 2021-2022
 */

#ifndef TST_TEST_MACROS_H__
#define TST_TEST_MACROS_H__

#include <stdbool.h>

#define TEST(SCALL) \
	do { \
		errno = 0; \
		TST_RET = SCALL; \
		TST_ERR = errno; \
	} while (0)

#define TEST_VOID(SCALL) \
	do { \
		errno = 0; \
		SCALL; \
		TST_ERR = errno; \
	} while (0)

extern long TST_RET;
extern int TST_ERR;
extern int TST_PASS;

extern void *TST_RET_PTR;

#define TESTPTR(SCALL) \
	do { \
		errno = 0; \
		TST_RET_PTR = (void*)SCALL; \
		TST_ERR = errno; \
	} while (0)


#define TST_2_(_1, _2, ...) _2

#define TST_FMT_(FMT, _1, ...) FMT, ##__VA_ARGS__

#define TST_MSG_(RES, FMT, SCALL, ...) \
	tst_res_(__FILE__, __LINE__, RES, \
		TST_FMT_(TST_2_(dummy, ##__VA_ARGS__, SCALL) FMT, __VA_ARGS__))

#define TST_MSGP_(RES, FMT, PAR, SCALL, ...) \
	tst_res_(__FILE__, __LINE__, RES, \
		TST_FMT_(TST_2_(dummy, ##__VA_ARGS__, SCALL) FMT, __VA_ARGS__), PAR)

#define TST_MSGP2_(RES, FMT, PAR, PAR2, SCALL, ...) \
	tst_res_(__FILE__, __LINE__, RES, \
		TST_FMT_(TST_2_(dummy, ##__VA_ARGS__, SCALL) FMT, __VA_ARGS__), PAR, PAR2)

#define TST_EXP_POSITIVE__(SCALL, SSCALL, ...)                                 \
	do {                                                                   \
		TEST(SCALL);                                                   \
		                                                               \
		TST_PASS = 0;                                                  \
		                                                               \
		if (TST_RET == -1) {                                           \
			TST_MSG_(TFAIL | TTERRNO, " failed",                   \
			         SSCALL, ##__VA_ARGS__);                       \
			break;                                                 \
		}                                                              \
		                                                               \
		if (TST_RET < 0) {                                             \
			TST_MSGP_(TFAIL | TTERRNO, " invalid retval %ld",      \
			          TST_RET, SSCALL, ##__VA_ARGS__);             \
			break;                                                 \
		}                                                              \
                                                                               \
		TST_PASS = 1;                                                  \
                                                                               \
	} while (0)

#define TST_EXP_POSITIVE_(SCALL, SSCALL, ...)                                  \
	({                                                                     \
		TST_EXP_POSITIVE__(SCALL, SSCALL, ##__VA_ARGS__);              \
		TST_RET;                                                       \
	})

#define TST_EXP_POSITIVE(SCALL, ...)                                           \
	({                                                                     \
		TST_EXP_POSITIVE__(SCALL, #SCALL, ##__VA_ARGS__);              \
		                                                               \
		if (TST_PASS) {                                                \
			TST_MSGP_(TPASS, " returned %ld",                      \
			          TST_RET, #SCALL, ##__VA_ARGS__);             \
		}                                                              \
		                                                               \
		TST_RET;                                                       \
	})

#define TST_EXP_FD_SILENT(SCALL, ...)	TST_EXP_POSITIVE_(SCALL, #SCALL, ##__VA_ARGS__)

#define TST_EXP_FD(SCALL, ...)                                                 \
	({                                                                     \
		TST_EXP_POSITIVE__(SCALL, #SCALL, ##__VA_ARGS__);              \
		                                                               \
		if (TST_PASS)                                                  \
			TST_MSGP_(TPASS, " returned fd %ld", TST_RET,          \
				#SCALL, ##__VA_ARGS__);                        \
		                                                               \
		TST_RET;                                                       \
	})

#define TST_EXP_FD_OR_FAIL(SCALL, ERRNO, ...)                                  \
	({                                                                     \
		if (ERRNO)                                                     \
			TST_EXP_FAIL(SCALL, ERRNO, ##__VA_ARGS__);             \
		else                                                           \
			TST_EXP_FD(SCALL, ##__VA_ARGS__);                      \
		                                                               \
		TST_RET;                                                       \
	})

#define TST_EXP_PID_SILENT(SCALL, ...)	TST_EXP_POSITIVE_(SCALL, #SCALL, ##__VA_ARGS__)

#define TST_EXP_PID(SCALL, ...)                                                \
	({                                                                     \
		TST_EXP_POSITIVE__(SCALL, #SCALL, ##__VA_ARGS__);              \
									       \
		if (TST_PASS)                                                  \
			TST_MSGP_(TPASS, " returned pid %ld", TST_RET,         \
				#SCALL, ##__VA_ARGS__);                        \
		                                                               \
		TST_RET;                                                       \
	})

#define TST_EXP_VAL_SILENT_(SCALL, VAL, SSCALL, ...)                           \
	do {                                                                   \
		TEST(SCALL);                                                   \
		                                                               \
		TST_PASS = 0;                                                  \
		                                                               \
		if (TST_RET != VAL) {                                          \
			TST_MSGP2_(TFAIL | TTERRNO, " retval %ld != %ld",      \
			          TST_RET, (long)VAL, SSCALL, ##__VA_ARGS__);  \
			break;                                                 \
		}                                                              \
		                                                               \
		TST_PASS = 1;                                                  \
		                                                               \
	} while (0)

#define TST_EXP_VAL_SILENT(SCALL, VAL, ...) TST_EXP_VAL_SILENT_(SCALL, VAL, #SCALL, ##__VA_ARGS__)

#define TST_EXP_VAL(SCALL, VAL, ...)                                           \
	do {                                                                   \
		TST_EXP_VAL_SILENT_(SCALL, VAL, #SCALL, ##__VA_ARGS__);        \
		                                                               \
		if (TST_PASS)                                                  \
			TST_MSG_(TPASS, " passed", #SCALL, ##__VA_ARGS__);     \
			                                                       \
	} while(0)

#define TST_EXP_PASS_SILENT_(SCALL, SSCALL, ...)                               \
	do {                                                                   \
		TEST(SCALL);                                                   \
		                                                               \
		TST_PASS = 0;                                                  \
		                                                               \
		if (TST_RET == -1) {                                           \
			TST_MSG_(TFAIL | TTERRNO, " failed",                   \
			         SSCALL, ##__VA_ARGS__);                       \
		        break;                                                 \
		}                                                              \
		                                                               \
		if (TST_RET != 0) {                                            \
			TST_MSGP_(TFAIL | TTERRNO, " invalid retval %ld",      \
			          TST_RET, SSCALL, ##__VA_ARGS__);             \
			break;                                                 \
		}                                                              \
                                                                               \
		TST_PASS = 1;                                                  \
                                                                               \
	} while (0)

#define TST_EXP_PASS_SILENT_PTR_(SCALL, SSCALL, FAIL_PTR_VAL, ...)             \
	do {                                                                   \
		TESTPTR(SCALL);                                                \
		                                                               \
		TST_PASS = 0;                                                  \
		                                                               \
		if (TST_RET_PTR == FAIL_PTR_VAL) {                             \
			TST_MSG_(TFAIL | TTERRNO, " failed",                   \
			         SSCALL, ##__VA_ARGS__);                       \
		        break;                                                 \
		}                                                              \
		                                                               \
		if (TST_RET != 0) {                                            \
			TST_MSGP_(TFAIL | TTERRNO, " invalid retval %ld",      \
			          TST_RET, SSCALL, ##__VA_ARGS__);             \
			break;                                                 \
		}                                                              \
                                                                               \
		TST_PASS = 1;                                                  \
                                                                               \
	} while (0)

#define TST_EXP_PASS_SILENT(SCALL, ...) TST_EXP_PASS_SILENT_(SCALL, #SCALL, ##__VA_ARGS__)

#define TST_EXP_PASS(SCALL, ...)                                               \
	do {                                                                   \
		TST_EXP_PASS_SILENT_(SCALL, #SCALL, ##__VA_ARGS__);            \
		                                                               \
		if (TST_PASS)                                                  \
			TST_MSG_(TPASS, " passed", #SCALL, ##__VA_ARGS__);     \
	} while (0)                                                            \

#define TST_EXP_PASS_PTR_(SCALL, SSCALL, FAIL_PTR_VAL, ...)                    \
	do {                                                                   \
		TST_EXP_PASS_SILENT_PTR_(SCALL, SSCALL,                        \
					FAIL_PTR_VAL, ##__VA_ARGS__);          \
		                                                               \
		if (TST_PASS)                                                  \
			TST_MSG_(TPASS, " passed", #SCALL, ##__VA_ARGS__);     \
	} while (0)

#define TST_EXP_PASS_PTR_NULL(SCALL, ...)                                      \
               TST_EXP_PASS_PTR_(SCALL, #SCALL, NULL, ##__VA_ARGS__);

#define TST_EXP_PASS_PTR_VOID(SCALL, ...)                                      \
               TST_EXP_PASS_PTR_(SCALL, #SCALL, (void *)-1, ##__VA_ARGS__);

/*
 * Returns true if err is in the exp_err array.
 */
bool tst_errno_in_set(int err, const int *exp_errs, int exp_errs_cnt);

/*
 * Fills in the buf with the errno names in the exp_err set. The buf must be at
 * least 20 * exp_errs_cnt bytes long.
 */
const char *tst_errno_names(char *buf, const int *exp_errs, int exp_errs_cnt);

#define TST_EXP_FAIL_SILENT_(PASS_COND, SCALL, SSCALL, ERRNOS, ERRNOS_CNT, ...)\
	do {                                                                   \
		TEST(SCALL);                                                   \
		                                                               \
		TST_PASS = 0;                                                  \
		                                                               \
		if (PASS_COND) {                                               \
			TST_MSG_(TFAIL, " succeeded", SSCALL, ##__VA_ARGS__);  \
		        break;                                                 \
		}                                                              \
		                                                               \
		if (TST_RET != -1) {                                           \
			TST_MSGP_(TFAIL | TTERRNO, " invalid retval %ld",      \
			          TST_RET, SSCALL, ##__VA_ARGS__);             \
			break;                                                 \
		}                                                              \
		                                                               \
		if (tst_errno_in_set(TST_ERR, ERRNOS, ERRNOS_CNT)) {           \
			TST_PASS = 1;                                          \
		} else {                                                       \
			char tst_str_buf__[ERRNOS_CNT * 20];                   \
			TST_MSGP_(TFAIL | TTERRNO, " expected %s",             \
				  tst_errno_names(tst_str_buf__,               \
						  ERRNOS, ERRNOS_CNT),         \
				  SSCALL, ##__VA_ARGS__);                      \
		}                                                              \
	} while (0)

#define TST_EXP_FAIL_SILENT_PTR_(SCALL, SSCALL, FAIL_PTR_VAL,                  \
	ERRNOS, ERRNOS_CNT, ...)                                               \
	do {                                                                   \
		TESTPTR(SCALL);                                                \
		                                                               \
		TST_PASS = 0;                                                  \
		                                                               \
		if (TST_RET_PTR != FAIL_PTR_VAL) {                             \
			TST_MSG_(TFAIL, " succeeded", SSCALL, ##__VA_ARGS__);  \
		        break;                                                 \
		}                                                              \
		                                                               \
		if (!tst_errno_in_set(TST_ERR, ERRNOS, ERRNOS_CNT)) {          \
			char tst_str_buf__[ERRNOS_CNT * 20];                   \
			TST_MSGP_(TFAIL | TTERRNO, " expected %s",             \
				  tst_errno_names(tst_str_buf__,               \
						  ERRNOS, ERRNOS_CNT),         \
				  SSCALL, ##__VA_ARGS__);                      \
			break;                                                 \
		}                                                              \
                                                                               \
		TST_PASS = 1;                                                  \
                                                                               \
	} while (0)

#define TST_EXP_FAIL_PTR_(SCALL, SSCALL, FAIL_PTR_VAL,                         \
	ERRNOS, ERRNOS_CNT, ...)                                               \
	do {                                                                   \
		TST_EXP_FAIL_SILENT_PTR_(SCALL, SSCALL, FAIL_PTR_VAL,          \
	        ERRNOS, ERRNOS_CNT, ##__VA_ARGS__);                            \
		if (TST_PASS)                                                  \
			TST_MSG_(TPASS | TTERRNO, " ", SSCALL, ##__VA_ARGS__); \
	} while (0)


#define TST_EXP_FAIL_ARR_(SCALL, SSCALL, EXP_ERRS, EXP_ERRS_CNT, ...)          \
	do {                                                                   \
		TST_EXP_FAIL_SILENT_(TST_RET == 0, SCALL, SSCALL,              \
			EXP_ERRS, EXP_ERRS_CNT, ##__VA_ARGS__);                \
		if (TST_PASS)                                                  \
			TST_MSG_(TPASS | TTERRNO, " ", SSCALL, ##__VA_ARGS__); \
	} while (0)

#define TST_EXP_FAIL(SCALL, EXP_ERR, ...)                                      \
	do {                                                                   \
		int tst_exp_err__ = EXP_ERR;                                   \
		TST_EXP_FAIL_ARR_(SCALL, #SCALL, &tst_exp_err__, 1,            \
                                  ##__VA_ARGS__);                              \
	} while (0)

#define TST_EXP_FAIL_ARR(SCALL, EXP_ERRS, EXP_ERRS_CNT, ...)                   \
		TST_EXP_FAIL_ARR_(SCALL, #SCALL, EXP_ERRS,                     \
				  EXP_ERRS_CNT, ##__VA_ARGS__);

#define TST_EXP_FAIL2_ARR_(SCALL, SSCALL, EXP_ERRS, EXP_ERRS_CNT, ...)         \
	do {                                                                   \
		TST_EXP_FAIL_SILENT_(TST_RET >= 0, SCALL, SSCALL,              \
			EXP_ERRS, EXP_ERRS_CNT, ##__VA_ARGS__);                \
		if (TST_PASS)                                                  \
			TST_MSG_(TPASS | TTERRNO, " ", SSCALL, ##__VA_ARGS__); \
	} while (0)

#define TST_EXP_FAIL2_ARR(SCALL, EXP_ERRS, EXP_ERRS_CNT, ...)                \
		TST_EXP_FAIL2_ARR_(SCALL, #SCALL, EXP_ERRS,                    \
		                  EXP_ERRS_CNT, ##__VA_ARGS__);

#define TST_EXP_FAIL_PTR_NULL(SCALL, EXP_ERR, ...)                          \
	do {                                                                   \
		int tst_exp_err__ = EXP_ERR;                                   \
		TST_EXP_FAIL_PTR_(SCALL, #SCALL, NULL,                         \
			&tst_exp_err__, 1, ##__VA_ARGS__);                     \
	} while (0)

#define TST_EXP_FAIL_PTR_NULL_ARR(SCALL, EXP_ERRS, EXP_ERRS_CNT, ...)      \
		TST_EXP_FAIL_PTR_(SCALL, #SCALL, NULL,                         \
			EXP_ERRS, EXP_ERRS_CNT, ##__VA_ARGS__);

#define TST_EXP_FAIL_PTR_VOID(SCALL, EXP_ERR, ...)                         \
	do {                                                                   \
		int tst_exp_err__ = EXP_ERR;                                   \
		TST_EXP_FAIL_PTR_(SCALL, #SCALL, (void *)-1,                   \
			&tst_exp_err__, 1, ##__VA_ARGS__);                     \
	} while (0)

#define TST_EXP_FAIL_PTR_VOID_ARR(SCALL, EXP_ERRS, EXP_ERRS_CNT, ...)      \
		TST_EXP_FAIL_PTR_(SCALL, #SCALL, (void *)-1,                   \
			EXP_ERRS, EXP_ERRS_CNT, ##__VA_ARGS__);

#define TST_EXP_FAIL2(SCALL, EXP_ERR, ...)                                     \
	do {                                                                   \
		int tst_exp_err__ = EXP_ERR;                                   \
		TST_EXP_FAIL2_ARR_(SCALL, #SCALL, &tst_exp_err__, 1,           \
                                  ##__VA_ARGS__);                              \
	} while (0)

#define TST_EXP_FAIL_SILENT(SCALL, EXP_ERR, ...)                               \
	do {                                                                   \
		int tst_exp_err__ = EXP_ERR;                                   \
		TST_EXP_FAIL_SILENT_(TST_RET == 0, SCALL, #SCALL,              \
			&tst_exp_err__, 1, ##__VA_ARGS__);                     \
	} while (0)

#define TST_EXP_FAIL2_SILENT(SCALL, EXP_ERR, ...)                              \
	do {                                                                   \
		int tst_exp_err__ = EXP_ERR;                                   \
		TST_EXP_FAIL_SILENT_(TST_RET >= 0, SCALL, #SCALL,              \
			&tst_exp_err__, 1, ##__VA_ARGS__);                     \
	} while (0)

#define TST_EXP_EXPR(EXPR, FMT, ...)						\
	tst_res_(__FILE__, __LINE__, (EXPR) ? TPASS : TFAIL, "Expect: " FMT, ##__VA_ARGS__);

#define TST_EXP_EQ_(VAL_A, SVAL_A, VAL_B, SVAL_B, TYPE, PFS) do {\
	TYPE tst_tmp_a__ = VAL_A; \
	TYPE tst_tmp_b__ = VAL_B; \
	if (tst_tmp_a__ == tst_tmp_b__) { \
		tst_res_(__FILE__, __LINE__, TPASS, \
			SVAL_A " == " SVAL_B " (" PFS ")", tst_tmp_a__); \
	} else { \
		tst_res_(__FILE__, __LINE__, TFAIL, \
			SVAL_A " (" PFS ") != " SVAL_B " (" PFS ")", \
			tst_tmp_a__, tst_tmp_b__); \
	} \
} while (0)

#define TST_EXP_EQ_LI(VAL_A, VAL_B) \
		TST_EXP_EQ_(VAL_A, #VAL_A, VAL_B, #VAL_B, long long, "%lli")

#define TST_EXP_EQ_LU(VAL_A, VAL_B) \
		TST_EXP_EQ_(VAL_A, #VAL_A, VAL_B, #VAL_B, unsigned long long, "%llu")

#define TST_EXP_EQ_SZ(VAL_A, VAL_B) \
		TST_EXP_EQ_(VAL_A, #VAL_A, VAL_B, #VAL_B, size_t, "%zu")

#define TST_EXP_EQ_SSZ(VAL_A, VAL_B) \
		TST_EXP_EQ_(VAL_A, #VAL_A, VAL_B, #VAL_B, ssize_t, "%zi")

#endif	/* TST_TEST_MACROS_H__ */
