/*
 * Copyright (c) 2009 Corey Tabaka
 * Copyright (c) 2014 Intel Corporation
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

/*
 * System Selectors
 */
#define NULL_SELECTOR       0x00

/********* x86 selectors *********/
#define CODE_SELECTOR       0x08
#define DATA_SELECTOR       0x10
#define USER_CODE_32_SELECTOR   0x18
#define USER_DATA_32_SELECTOR   0x20

/******* x86-64 selectors ********/
#define CODE_64_SELECTOR    0x28
#define STACK_64_SELECTOR   0x30
#define USER_CODE_64_SELECTOR   0x38
#define USER_DATA_64_SELECTOR   0x40

/* base selector for a list of TSSes, one per cpu (SMP_MAX_CPUS) */
#define TSS_SELECTOR        0x48

/* code/data segment types (S = 1) */
/* bit 0 is accessed */
#define SEG_TYPE_DATA_RO             0x0
#define SEG_TYPE_DATA_RW             0x2
#define SEG_TYPE_DATA_RO_EXPAND_DOWN 0x4
#define SEG_TYPE_DATA_RW_EXPAND_DOWN 0x6
#define SEG_TYPE_CODE_XO             0x8
#define SEG_TYPE_CODE_RO             0xa
#define SEG_TYPE_CODE_XO_CONFORMING  0xc
#define SEG_TYPE_CODE_RO_CONFORMING  0xe

/* system segment types (S = 0) */
#define SEG_TYPE_TSS_16       0x1
#define SEG_TYPE_LDT          0x2
#define SEG_TYPE_TSS_16_BUSY  0x3
#define SEG_TYPE_CALL_GATE_16 0x4
#define SEG_TYPE_TASK_GATE    0x5
#define SEG_TYPE_INT_GATE_16  0x6
#define SEG_TYPE_TRAP_GATE_16 0x7
#define SEG_TYPE_TSS          0x9
#define SEG_TYPE_TSS_BUSY     0xb
#define SEG_TYPE_CALL_GATE    0xc
#define SEG_TYPE_INT_GATE     0xe
#define SEG_TYPE_TRAP_GATE    0xf

#ifndef ASSEMBLY

#include <sys/types.h>

typedef uint16_t seg_sel_t;

void x86_set_gdt_descriptor(seg_sel_t sel, void *base, uint32_t limit,
                     uint8_t present, uint8_t ring, uint8_t sys, uint8_t type, uint8_t gran, uint8_t bits);

#endif
