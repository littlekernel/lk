#pragma once
/*
 * Copyright (c) 2025-2026 Josh Cummings
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <lib/cbuf.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

// CIA-A registers
#define CIA_A_BASE   0xBFE000
#define CIA_A_PRA    0x001
#define CIA_A_PRB    0x101
#define CIA_A_DDRA   0x201
#define CIA_A_DDRB   0x301
#define CIA_A_TALO   0x401
#define CIA_A_TAHI   0x501
#define CIA_A_TBLO   0x601
#define CIA_A_TBHI   0x701
#define CIA_A_TODLO  0x801
#define CIA_A_TODMID 0x901
#define CIA_A_TODHI  0xA01
#define CIA_A_SDR    0xC01
#define CIA_A_ICR    0xD01
#define CIA_A_CRA    0xE01
#define CIA_A_CRB    0xF01

// CIA-A control flags
#define CIA_A_CRA_START  (1 << 0)
#define CIA_A_CRA_LOAD   (1 << 4)
#define CIA_A_CRA_SERIAL (1 << 6)

// CIA-B registers
#define CIA_B_BASE   0xBFD000
#define CIA_B_PRA    0x000
#define CIA_B_PRB    0x100
#define CIA_B_DDRA   0x200
#define CIA_B_DDRB   0x300
#define CIA_B_TALO   0x400
#define CIA_B_TAHI   0x500
#define CIA_B_TBLO   0x600
#define CIA_B_TBHI   0x700
#define CIA_B_TODLO  0x800
#define CIA_B_TODMID 0x900
#define CIA_B_TODHI  0xA00
#define CIA_B_SDR    0xC00
#define CIA_B_ICR    0xD00
#define CIA_B_CRA    0xE00
#define CIA_B_CRB    0xF00

// CIA-B control flags
#define CIA_B_CRA_START  (1 << 0)
#define CIA_B_CRA_LOAD   (1 << 4)
#define CIA_B_CRA_SERIAL (1 << 6)

#define DMACON  0x096
#define COP1LCH 0x080
#define COP1LCL 0x082
#define COPJMP1 0x088

#define BPLCON0 0x100
#define BPLCON1 0x102
#define BPLCON2 0x104

#define BPL1PTH 0x0E0
#define BPL1PTL 0x0E2
#define BPL1MOD 0x108
#define BPL2MOD 0x10A

#define DIWSTRT 0x08E
#define DIWSTOP 0x090
#define DDFSTRT 0x092
#define DDFSTOP 0x094

#define COLOR00 0x180
#define COLOR01 0x182

/*
 * Interrupts are defined here with 1-based indexing, for simplicity/readability.
 *
 * We treat/define CIA interrupt numbers like a continuation of Paula's.
 * 
 */
enum {
   // Paula
   INTERRUPT_TBE = 1,
   INTERRUPT_DSKBLK,
   INTERRUPT_SOFTINT,
   INTERRUPT_PORTS,
   INTERRUPT_COPER,
   INTERRUPT_VERTB,
   INTERRUPT_BLIT,
   INTERRUPT_AUD2, 
   INTERRUPT_AUD0, 
   INTERRUPT_AUD3, 
   INTERRUPT_AUD1, 
   INTERRUPT_RBF, 
   INTERRUPT_DSKSYN,
   INTERRUPT_EXTER,

   // CIA A
   INTERRUPT_TIMERA_A,
   INTERRUPT_TIMERB_A,
   INTERRUPT_TOD_A,
   INTERRUPT_SERP_A,
   INTERRUPT_FLAG_A,

   // CIA B
   INTERRUPT_TIMERA_B,
   INTERRUPT_TIMERB_B,
   INTERRUPT_TOD_B,
   INTERRUPT_SERP_B,
   INTERRUPT_FLAG_B,
};

// Used for dealing with Amiga interrupt multiplexing
static const uint16_t irq_level_map[] = {
    0x0007, // CPU level 1
    0x300C, // CPU level 2
    0x0070, // CPU level 3
    0x0780, // CPU level 4
    0x1800, // CPU level 5
    0x2000, // CPU level 6
};

// Paula interrupt register offsets
enum {
    INTREQ = 0x9c,
    INTENA = 0x9a,
    INTREQR = 0x1e,
    INTENAR = 0x1c,
};

// Fourteen chipset-level interrupts from Paula, and five per CIA
enum {
    NUM_IRQS_TOTAL = 24,
    NUM_IRQS_PAULA = 14,
    NUM_IRQS_CIA = 5,
};

// Interrupts originating from each CIA are multiplexed/nested within CPU &
// chipset-level IRQs. These values correspond to Paula's interrupt bits.
enum {
    CIA_A_MUX_LEVEL = 3,  // 'PORTS' IRQ, CIA-A and INT2
    CIA_B_MUX_LEVEL = 13, // 'EXTER' IRQ, CIA-B and INT6
};

void cia_timer_init(void);
void platform_serial_init(void);
void platform_keyboard_init(cbuf_t *buffer);
int platform_dgetc(char *c, bool wait);

void uart_putc(char c);
int uart_getc(char *c, bool wait);
void dputc(char c);

status_t clear_interrupt(unsigned int bit);
