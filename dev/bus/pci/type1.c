/*
 * Copyright (c) 2009 Corey Tabaka
 * Copyright (c) 2020 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/debug.h>
#include <lk/err.h>
#include <stdlib.h>
#include <string.h>
#include <kernel/thread.h>
#include <kernel/spinlock.h>
#include <dev/bus/pci.h>
#include <lk/trace.h>

#include "pci_priv.h"

#if ARCH_X86
// Only supported on x86

#include <arch/x86/descriptor.h>
#include <arch/x86.h>

#define LOCAL_TRACE 0

static uint16_t type1_read_byte(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    /* create configuration address as per Figure 1 */
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) |
                                  (func << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));

    /* write out the address */
    outpd(0xCF8, address);

    /* read in the data 32 bits at a time and then shift over our byte */
    uint8_t tmp = ((inpd(0xCFC) >> ((offset & 3) * 8)) & 0xffff);
    return tmp;
}

static uint16_t type1_read_half(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    /* create configuration address as per Figure 1 */
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) |
                                  (func << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));

    /* write out the address */
    outpd(0xCF8, address);

    /* read in the data 32 bits at a time */
    /* (offset & 2) * 8) = 0 will choose the first word of the 32 bits register */
    uint16_t tmp = ((inpd(0xCFC) >> ((offset & 2) * 8)) & 0xffff);
    return tmp;
}

static uint32_t type1_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    /* create configuration address as per Figure 1 */
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) |
                                  (func << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));

    /* write out the address */
    outpd(0xCF8, address);

    /* read in the data 32 bits at a time */
    /* (offset & 2) * 8) = 0 will choose the first word of the 32 bits register */
    uint32_t tmp = inpd(0xCFC);
    return tmp;
}

static int type1_read_config_word(const pci_location_t *state, uint32_t reg, uint32_t *value) {
    LTRACEF("state bus %#hhx dev_fn %#hhx reg %#x\n", state->bus, state->dev_fn, reg);
    *value = type1_read_word(state->bus, state->dev_fn >> 3, state->dev_fn & 0x7, reg);
    return NO_ERROR;
}

static int type1_read_config_half(const pci_location_t *state, uint32_t reg, uint16_t *value) {
    LTRACEF("state bus %#hhx dev_fn %#hhx reg %#x\n", state->bus, state->dev_fn, reg);
    *value = type1_read_half(state->bus, state->dev_fn >> 3, state->dev_fn & 0x7, reg);
    return NO_ERROR;
}

static int type1_read_config_byte(const pci_location_t *state, uint32_t reg, uint8_t *value) {
    LTRACEF("state bus %#hhx dev_fn %#hhx reg %#x\n", state->bus, state->dev_fn, reg);
    *value = type1_read_half(state->bus, state->dev_fn >> 3, state->dev_fn & 0x7, reg);
    return NO_ERROR;
}

int pci_type1_detect(void) {
    LTRACE_ENTRY;

    /* fill in config reading routines */
    g_pci_read_config_word = type1_read_config_word;
    g_pci_read_config_half = type1_read_config_half;
    g_pci_read_config_byte = type1_read_config_byte;

    /* we don't know how many busses there are */
    last_bus = 32; // TODO: find better version of this

    return NO_ERROR;
}

#else // !ARCH_X86

// Non x86 arches don't have this mechanism
int pci_type1_detect(void) {
    return ERR_NOT_SUPPORTED;
}

#endif
