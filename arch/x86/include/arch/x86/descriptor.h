/*
 * Copyright (c) 2009 Corey Tabaka
 * Copyright (c) 2014 Intel Corporation
 * Copyright (c) 2024 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

// System Selectors
#define NULL_SELECTOR       0x00

// ********* x86 selectors *********
// Laid out slightly differently based on 32 vs 64 bit mode
#if ARCH_X86_64

#define CODE_SELECTOR       0x08
#define CODE_64_SELECTOR    0x10
#define DATA_SELECTOR       0x18
#define USER_CODE_32_SELECTOR   0x20
#define USER_DATA_32_SELECTOR   0x28
#define USER_CODE_64_SELECTOR   0x30
#define USER_DATA_64_SELECTOR   0x38
#define TSS_SELECTOR_BASE       0x40

#elif ARCH_X86_32

#define CODE_SELECTOR       0x08
#define DATA_SELECTOR       0x10
#define USER_CODE_32_SELECTOR   0x18
#define USER_DATA_32_SELECTOR   0x20
#define TSS_SELECTOR_BASE       0x28

#else
#error unknown architecture
#endif

// Base selector for a gs segment per cpu (SMP_MAX_CPUS)
#define PERCPU_SELECTOR_BASE    (TSS_SELECTOR_BASE + 8 * SMP_MAX_CPUS)

// Worksheet of what the syscall instructions do which affects the GDT layout:
// SYSENTER
//     CS = IA32_SYSENTER_CS
//     SS = IA32_SYSENTER_CS + 8
// SYSEXIT 32
//     CS = IA32_SYSENTER_CS + 16
//     SS = IA32_SYSENTER_CS + 24
// SYSEXIT 64
//     CS = IA32_SYSENTER_CS + 32
//     SS = IA32_SYSENTER_CS + 40

// SYSCALL
//     CS = IA32_STAR.SYSCALL_CS
//     SS = IA32_STAR.SYSCALL_CS + 8
// SYSRET 32
//     CS = IA32_STAR.SYSRET_CS
//     SS = IA32_STAR.SYSRET_CS + 8
// SYSRET 64
//     CS = IA32_STAR.SYSRET_CS + 16
//     SS = IA32_STAR.SYSRET_CS + 8

// code/data segment types (S = 1)
// bit 0 is A (accessed)
// bit 1 is W (accessed)
// bit 2 is E (expand-down)
// bit 3 is data (0) vs code (1)

// data segment types:
#define SEG_TYPE_DATA_RO             0x0
#define SEG_TYPE_DATA_RW             0x2
#define SEG_TYPE_DATA_RO_EXPAND_DOWN 0x4
#define SEG_TYPE_DATA_RW_EXPAND_DOWN 0x6

// code segment types:
// bit 3 is C (conforming)
#define SEG_TYPE_CODE_XO             0x9
#define SEG_TYPE_CODE_RO             0xa
#define SEG_TYPE_CODE_XO_CONFORMING  0xc
#define SEG_TYPE_CODE_RO_CONFORMING  0xe

// system segment types (S = 0)
#define SEG_TYPE_TSS_16       0x1
#define SEG_TYPE_LDT          0x2 // usable in 64bit
#define SEG_TYPE_TSS_16_BUSY  0x3
#define SEG_TYPE_CALL_GATE_16 0x4
#define SEG_TYPE_TASK_GATE    0x5
#define SEG_TYPE_INT_GATE_16  0x6
#define SEG_TYPE_TRAP_GATE_16 0x7
#define SEG_TYPE_TSS          0x9 // usable in 64bit
#define SEG_TYPE_TSS_BUSY     0xb // usable in 64bit
#define SEG_TYPE_CALL_GATE    0xc // usable in 64bit
#define SEG_TYPE_INT_GATE     0xe // usable in 64bit
#define SEG_TYPE_TRAP_GATE    0xf // usable in 64bit

#ifndef ASSEMBLY

#include <sys/types.h>

typedef uint16_t seg_sel_t;

void x86_set_gdt_descriptor(seg_sel_t sel, void *base, uint32_t limit,
                     uint8_t present, uint8_t ring, uint8_t sys, uint8_t type, uint8_t gran, uint8_t bits);

#endif
