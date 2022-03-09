/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2021 SUSE LLC <mdoucha@suse.cz>
 *
 * Minimal test library for KVM tests
 */

#ifndef KVM_GUEST_H_
#define KVM_GUEST_H_

/*
 * These constants must match the value of corresponding constants defined
 * in tst_res_flags.h.
 */
#define TPASS	0	/* Test passed flag */
#define TFAIL	1	/* Test failed flag */
#define TBROK	2	/* Test broken flag */
#define TWARN	4	/* Test warning flag */
#define TINFO	16	/* Test information flag */
#define TCONF	32	/* Test not appropriate for configuration */

#define TST_TEST_TCONF(message) \
	void main(void) { tst_brk(TCONF, message); }

#define INTERRUPT_COUNT 32

typedef unsigned long size_t;
typedef long ssize_t;

typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef long long int64_t;
typedef unsigned long long uint64_t;
typedef unsigned long uintptr_t;

#define NULL ((void *)0)

void *memset(void *dest, int val, size_t size);
void *memzero(void *dest, size_t size);
void *memcpy(void *dest, const void *src, size_t size);

char *strcpy(char *dest, const char *src);
char *strcat(char *dest, const char *src);
size_t strlen(const char *str);

/* Exit the VM by looping on a HLT instruction forever */
void kvm_exit(void) __attribute__((noreturn));

/* Exit the VM using the HLT instruction but allow resume */
void kvm_yield(void);

void tst_res_(const char *file, const int lineno, int result,
	const char *message);
#define tst_res(result, msg) tst_res_(__FILE__, __LINE__, (result), (msg))

void tst_brk_(const char *file, const int lineno, int result,
	const char *message) __attribute__((noreturn));
#define tst_brk(result, msg) tst_brk_(__FILE__, __LINE__, (result), (msg))

void *tst_heap_alloc_aligned(size_t size, size_t align);
void *tst_heap_alloc(size_t size);

/* Arch dependent: */

struct kvm_interrupt_frame;

typedef int (*tst_interrupt_callback)(void *userdata,
	struct kvm_interrupt_frame *ifrm, unsigned long errcode);

extern const char *tst_interrupt_names[INTERRUPT_COUNT];

void tst_set_interrupt_callback(unsigned int vector,
	tst_interrupt_callback func, void *userdata);

/* Get the instruction pointer from interrupt frame */
uintptr_t kvm_get_interrupt_ip(const struct kvm_interrupt_frame *ifrm);

#endif /* KVM_GUEST_H_ */
