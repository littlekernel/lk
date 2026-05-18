/*
 * Copyright (c) 2026 Josh Cummings
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include "platform_p.h"
#include <lib/cbuf.h>
#include <lib/console.h>
#include <platform.h>
#include <platform/interrupts.h>

#define SCANCODE_LSHIFT 0x60
#define SCANCODE_RSHIFT 0x61

static volatile uint8_t *const cia_base = (volatile uint8_t *)CIA_A_BASE;

// clang-format off
// Amiga raw keycode -> ASCII (US layout). Unassigned entries are 0.
static const int KeyCodeSingleLower[128] = {
    [0x00] = '`', [0x01] = '1', [0x02] = '2', [0x03] = '3', [0x04] = '4', [0x05] = '5',
    [0x06] = '6', [0x07] = '7', [0x08] = '8', [0x09] = '9', [0x0a] = '0', [0x0b] = '-', 
    [0x0c] = '=', [0x0d] = '\\', [0x0e] = 0, [0x41] = '\b', [0x42] = '\t',

    [0x10] = 'q', [0x11] = 'w', [0x12] = 'e', [0x13] = 'r', [0x14] = 't', [0x15] = 'y',
    [0x16] = 'u', [0x17] = 'i', [0x18] = 'o', [0x19] = 'p', [0x1a] = '[', [0x1b] = ']',
    [0x44] = '\n', // Enter/Return

    [0x62] = 0,
    [0x20] = 'a', [0x21] = 's', [0x22] = 'd', [0x23] = 'f', [0x24] = 'g', [0x25] = 'h', 
    [0x26] = 'j', [0x27] = 'k', [0x28] = 'l', [0x29] = ';', [0x2a] = '\'', [0x31] = 'z',
    [0x32] = 'x', [0x33] = 'c', [0x34] = 'v', [0x35] = 'b', [0x36] = 'n', [0x37] = 'm',
    [0x38] = ',', [0x39] = '.', [0x3a] = '/',
    [0x40] = ' ', [0x45] = 0x1b, // Space & Esc

    // Keypad
    [0x0f] = '0', [0x1d] = '1', [0x1e] = '2', [0x1f] = '3', [0x2d] = '4', [0x2e] = '5',
    [0x2f] = '6', [0x3d] = '7', [0x3e] = '8', [0x3f] = '9', [0x3c] = '.', [0x4a] = '-',
    [0x5a] = '(', [0x5b] = ')', [0x5c] = '/', [0x5e] = '+', [0x43] = '\n',

    [0x2b] = 0,          // (international key on some layouts)
    [0x30] = 0,          // (international key on some layouts)
};

static const int KeyCodeSingleUpper[128] = {
    [0x00] = '~', [0x01] = '!', [0x02] = '@', [0x03] = '#', [0x04] = '$', [0x05] = '%',
    [0x06] = '^', [0x07] = '&', [0x08] = '*', [0x09] = '(', [0x0a] = ')', [0x0b] = '_',
    [0x0c] = '+', [0x0d] = '|', [0x0e] = 0, [0x0f] = '0',         // keypad 0 (optional)

    [0x10] = 'Q', [0x11] = 'W', [0x12] = 'E', [0x13] = 'R', [0x14] = 'T', [0x15] = 'Y',
    [0x16] = 'U', [0x17] = 'I', [0x18] = 'O', [0x19] = 'P', [0x1a] = '{', [0x1b] = '}', 
    [0x1c] = '\n', [0x1d] = 0, [0x1e] = 0, [0x1f] = 0,

    [0x20] = 'A', [0x21] = 'S', [0x22] = 'D', [0x23] = 'F', [0x24] = 'G', [0x25] = 'H',
    [0x26] = 'J', [0x27] = 'K', [0x28] = 'L', [0x29] = ':', [0x2a] = '"', [0x2b] = 0,
    [0x2c] = 0, [0x2d] = 0, [0x2e] = 0, [0x2f] = 0, [0x30] = 0,

    [0x31] = 'Z', [0x32] = 'X', [0x33] = 'C', [0x34] = 'V', [0x35] = 'B', [0x36] = 'N',
    [0x37] = 'M', [0x38] = '<', [0x39] = '>', [0x3a] = '?', [0x3b] = 0, [0x3c] = ' ',
};
// clang-format on

static bool key_lshift;
static bool key_rshift;

static cbuf_t *key_buf;

static inline uint8_t decode_key(uint8_t key) {
    return ~((key >> 1) | (key << 7));
}

static bool generate_esc_seq(cbuf_t *buf, char a, char b, char c) {
    char esc_seq[3] = {a, b, c};

    if (cbuf_space_avail(buf) < sizeof(esc_seq)) {
        return false;
    }

    return cbuf_write(buf, esc_seq, sizeof(esc_seq), false) == sizeof(esc_seq);
}

static bool cia_process_scode(uint8_t scode) {
    bool resched = false;
    bool keyUpBit;
    int keyCode;

    keyUpBit = (scode & 0x80) != 0;
    scode &= 0x7f;

    if (scode == SCANCODE_LSHIFT) {
        key_lshift = !keyUpBit;
    }

    if (scode == SCANCODE_RSHIFT) {
        key_rshift = !keyUpBit;
    }

    if (!keyUpBit) {
        switch (scode) {
            case 0x4f: // LEFT
                resched = generate_esc_seq(key_buf, 0x1b, '[', 'D');
                break;
            case 0x4e: // RIGHT
                resched = generate_esc_seq(key_buf, 0x1b, '[', 'C');
                break;
            case 0x4c: // UP
                resched = generate_esc_seq(key_buf, 0x1b, '[', 'A');
                break;
            case 0x4d: // DOWN
                resched = generate_esc_seq(key_buf, 0x1b, '[', 'B');
                break;
        }
    }

    if (key_lshift || key_rshift) {
        keyCode = KeyCodeSingleUpper[scode];
    } else {
        keyCode = KeyCodeSingleLower[scode];
    }

    if (keyCode != 0 && !keyUpBit) {
        resched = cbuf_write_char(key_buf, keyCode, false);
    }

    cia_base[CIA_A_CRA] |= (1 << 6);
    cia_base[CIA_A_CRA] &= ~(1 << 6);

    return resched;
}

static enum handler_return cia_kbd_interrupt(void *arg) {
    bool resched = false;
    uint8_t sdr = cia_base[CIA_A_SDR];
    uint8_t scode = decode_key(sdr);

    resched = cia_process_scode(scode);

    return resched ? INT_RESCHEDULE : INT_NO_RESCHEDULE;
}

int platform_read_key(char *c) {
    return cbuf_read_char(key_buf, c, true);
}

void platform_keyboard_init(cbuf_t *buffer) {
    key_buf = buffer;

    cia_base[CIA_A_CRA] &= ~(1 << 6);        // Set SPMODE to input
    cia_base[CIA_A_ICR] = (1 << 7 | 1 << 3); // Set SP bit in ICR

    register_int_handler(INTERRUPT_SERP_A, cia_kbd_interrupt, NULL);
    unmask_interrupt(INTERRUPT_PORTS);
    unmask_interrupt(INTERRUPT_SERP_A);
}
