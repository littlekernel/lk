/*
 * Copyright (c) 2009 Corey Tabaka
 * Copyright (c) 2015 Intel Corporation
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <stdbool.h>
#include <sys/types.h>

#include <platform/pc/iomap.h>
#include <platform/pc/memmap.h>

/* NOTE: keep arch/x86/crt0.S in sync with these definitions */

/* interrupts */
#define INT_VECTORS 256

/* defined interrupts */
#define INT_BASE      0x20
#define INT_PIT       0x20
#define INT_KEYBOARD  0x21
#define INT_PIC2      0x22
#define INT_COM2_COM4 0x23
#define INT_COM1_COM3 0x24

#define INT_BASE2    0x28
#define INT_CMOSRTC  0x28
#define INT_PS2MOUSE 0x2c
#define INT_IDE0     0x2e
#define INT_IDE1     0x2f

/* dynamic interrupts are allocated in this range */
#define INT_DYNAMIC_START 0x30
#define INT_DYNAMIC_END   0xef

/* APIC vectors */
#define INT_APIC_TIMER 0xf0

/* PIC remap bases */
#define INT_PIC1_BASE 0x20
#define INT_PIC2_BASE 0x28

typedef struct pc_irq_route {
	uint source_irq;
	uint gsi;
	bool has_override;
	bool route_level_triggered;
	bool route_active_low;
	bool has_ioapic_redir;
	uint ioapic_id;
	uint8_t vector;
	uint8_t destination_apic_id;
	bool masked;
	bool level_triggered;
	bool active_low;
} pc_irq_route_t;

status_t pc_get_legacy_irq_route(uint source_irq, pc_irq_route_t *route);
