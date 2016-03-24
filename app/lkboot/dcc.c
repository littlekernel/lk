/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "lkboot.h"

#include <stdio.h>
#include <debug.h>
#include <string.h>
#include <compiler.h>
#include <err.h>
#include <assert.h>
#include <trace.h>
#include <stdlib.h>
#include <lib/cbuf.h>
#include <app/lkboot.h>
#include <arch/arm/dcc.h>
#include <kernel/vm.h>
#include <kernel/mutex.h>

#include "pdcc.h"

#define LOCAL_TRACE 0

static struct pdcc_buffer_descriptor buffer_desc __ALIGNED(256);
static paddr_t buffer_desc_phys;

#define DCC_BUFLEN 256

static uint8_t htod_buffer[DCC_BUFLEN] __ALIGNED(CACHE_LINE);
static uint8_t dtoh_buffer[DCC_BUFLEN] __ALIGNED(CACHE_LINE);

static uint htod_index;
static uint htod_pos;
static bool dtoh_filled;

static void send_pdcc_command(uint32_t opcode, uint32_t data)
{
    uint32_t word;

    word = PDCC_VALID |
        ((opcode & 0x7f) << PDCC_OPCODE_SHIFT) |
        (data & 0x00ffffff);

    // XXX may block forever
    LTRACEF("sending 0x%x\n", word);
    arm_dcc_write(&word, 1, INFINITE_TIME);
}

static void send_buffer_header(void)
{
    send_pdcc_command(PDCC_OP_BUF_HEADER, buffer_desc_phys / 256);
}

static void send_reset(void)
{
    send_pdcc_command(PDCC_OP_RESET, 0);
}

static void send_out_index_update(uint32_t index)
{
    send_pdcc_command(PDCC_OP_UPDATE_OUT_INDEX, index);
}

static void send_buffer_consumed(void)
{
    send_pdcc_command(PDCC_OP_CONSUMED_IN, 0);
}

#define DCC_PROCESS_RESET 1
static int dcc_process_opcode(uint32_t word)
{
    int ret = 0;

    if (word & PDCC_VALID) {
        uint32_t opcode = PDCC_OPCODE(word);
        uint32_t data = PDCC_DATA(word);
        LTRACEF("word 0x%x, opcode 0x%x, data 0x%x\n", word, opcode, data);
        switch (opcode) {
            case PDCC_OP_RESET:
                htod_index = 0;
                htod_pos = 0;
                dtoh_filled = false;

                // try to send the buffer header
                send_buffer_header();
                ret = DCC_PROCESS_RESET;
                break;
            case PDCC_OP_BUF_HEADER:
                // we shouldn't get this
                break;

            case PDCC_OP_UPDATE_OUT_INDEX:
                if (data > DCC_BUFLEN) {
                    // out of range
                    send_reset();
                } else {
                    htod_index = data;
                    htod_pos = 0;
                    arch_invalidate_cache_range((vaddr_t)htod_buffer, DCC_BUFLEN);
                }
                break;

            case PDCC_OP_CONSUMED_IN:
                arch_invalidate_cache_range((vaddr_t)dtoh_buffer, DCC_BUFLEN);
                dtoh_filled = false;
                break;
            default:
                TRACEF("bad opcode from host 0x%x\n", opcode);
                send_reset();
        }
    }

    return ret;
}

static ssize_t dcc_read(void *unused, void *_data, size_t len)
{
    unsigned char *data = _data;
    size_t pos = 0;
    uint32_t dcc;

    LTRACEF("buf %p, len %zu, htod_pos %u, htod_index %u\n", _data, len, htod_pos, htod_index);

    lk_time_t timeout = 0; // first dcc command should be with no timeout
    while (pos < len) {
        // process a dcc command
        ssize_t err = arm_dcc_read(&dcc, 1, timeout);
        if (err > 0) {
            err = dcc_process_opcode(dcc);
            if (err == DCC_PROCESS_RESET) {
                return ERR_IO;
            }
        }

        // see if there is any data in the incoming buffer
        if (htod_index > 0) {
            size_t tocopy = MIN(htod_index - htod_pos, len - pos);

            memcpy(&data[pos], &htod_buffer[htod_pos], tocopy);
            pos += tocopy;
            htod_pos += tocopy;

            // if we consumed everything, tell the host we're done with the buffer
            if (htod_pos == htod_index) {
                send_buffer_consumed();
                htod_index = 0;
                htod_pos = 0;
                arch_invalidate_cache_range((vaddr_t)htod_buffer, DCC_BUFLEN);
            }
        }

        timeout = 1000;
    }

    return 0;
}

static ssize_t dcc_write(void *unused, const void *_data, size_t len)
{
    const unsigned char *data = _data;
    size_t pos = 0;

    LTRACEF("buf %p, len %zu\n", _data, len);

    while (pos < len) {
        LTRACEF("pos %zu, len %zu, dtoh_filled %d\n", pos, len, dtoh_filled);
        if (!dtoh_filled) {
            // put as much data as we can in the outgoing buffer
            size_t tocopy = MIN(len, DCC_BUFLEN);

            LTRACEF("tocopy %zu\n", tocopy);
            memcpy(dtoh_buffer, data, tocopy);
            arch_clean_cache_range((vaddr_t)dtoh_buffer, DCC_BUFLEN);
            send_out_index_update(tocopy);
            dtoh_filled = true;

            pos += tocopy;
        }

        // process a dcc command
        uint32_t dcc;
        ssize_t err = arm_dcc_read(&dcc, 1, 1000);
        if (err > 0) {
            err = dcc_process_opcode(dcc);
            if (err == DCC_PROCESS_RESET) {
                return ERR_IO;
            }
        }
    }

    return pos;
}

lkb_t *lkboot_check_dcc_open(void)
{
    lkb_t *lkb = NULL;

    // read a dcc op and process it
    {
        uint32_t dcc;
        ssize_t err = arm_dcc_read(&dcc, 1, 0);
        if (err > 0) {
            err = dcc_process_opcode(dcc);
        }
    }

    if (htod_index > 0) {
        // we have data, construct a lkb and return it
        LTRACEF("we have data on dcc, starting command handler\n");
        lkb = lkboot_create_lkb(NULL, dcc_read, dcc_write);
    }

    return lkb;
}

void lkboot_dcc_init(void)
{
    paddr_t pa;
    __UNUSED status_t err;

    buffer_desc.version = PDCC_VERSION;

    pa = vaddr_to_paddr(htod_buffer);
    DEBUG_ASSERT(pa);

    buffer_desc.htod_buffer_phys = pa;
    buffer_desc.htod_buffer_len = DCC_BUFLEN;

    pa = vaddr_to_paddr(dtoh_buffer);
    DEBUG_ASSERT(pa);

    buffer_desc.dtoh_buffer_phys = pa;
    buffer_desc.dtoh_buffer_len = DCC_BUFLEN;

    buffer_desc_phys = vaddr_to_paddr(&buffer_desc);
    DEBUG_ASSERT(buffer_desc_phys);

    arch_clean_cache_range((vaddr_t)&buffer_desc, sizeof(buffer_desc));
}

