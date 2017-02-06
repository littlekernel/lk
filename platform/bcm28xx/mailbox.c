/*
 * Copyright (c) 2017 Eric Holland
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

#include <reg.h>
#include <err.h>
#include <debug.h>
#include <trace.h>

#include <arch.h>
#include <platform.h>
#include <arch/ops.h>
#include <kernel/vm.h>
#include <dev/display.h>

#include <platform/bcm28xx.h>
#include <platform/mailbox.h>


static volatile uint32_t* mailbox_regs = (uint32_t*)ARM0_MAILBOX_BASE;
static fb_mbox_t fb_desc __ALIGNED(16);

static inline void* vc_bus_to_kvaddr(uint32_t bus_addr) {
    return (paddr_to_kvaddr(bus_addr & 0x3fffffff));
}

static inline uint32_t kvaddr_to_vc_bus(addr_t kvaddr) {
    return (uint32_t)((kvaddr & 0x3fffffff)+BCM_SDRAM_BUS_ADDR_BASE);
}

#define MAILBOX_WAIT_TIMEOUT_US 500000
#define MAILBOX_MAX_READ_ATTEMPTS 20

static status_t mailbox_write(const enum mailbox_channel ch, uint32_t value) {
    value = value | ch;

    lk_time_t now = current_time();

    // Wait for there to be space in the FIFO.
    while (mailbox_regs[MAILBOX_STATUS] & MAILBOX_FULL) {
        if ( (now + MAILBOX_WAIT_TIMEOUT_US) < current_time()) {
            return ERR_TIMED_OUT;
        }
    }

    // Write the value to the mailbox.
    mailbox_regs[MAILBOX_WRITE] = value;

    return NO_ERROR;
}

static status_t mailbox_read(enum mailbox_channel ch, uint32_t* result) {
    uint32_t local_result = 0;
    uint32_t attempts = 0;

    lk_time_t deadline;

    do {
        deadline = current_time() + MAILBOX_WAIT_TIMEOUT_US;
        while (mailbox_regs[MAILBOX_STATUS] & MAILBOX_EMPTY) {
            if (current_time() > deadline)
                return ERR_TIMED_OUT;
        }

        local_result = mailbox_regs[MAILBOX_READ];

        attempts++;

    } while ((((local_result)&0xF) != ch) && (attempts < MAILBOX_MAX_READ_ATTEMPTS));

    *result = (local_result);

    return attempts < MAX_MAILBOX_READ_ATTEMPTS ? NO_ERROR : ERR_IO;
}


static status_t mailbox_get_framebuffer(fb_mbox_t* fb_desc) {
    status_t ret = NO_ERROR;

        arch_clean_cache_range((addr_t)fb_desc,sizeof(fb_mbox_t));

        ret = mailbox_write(ch_framebuffer, kvaddr_to_vc_bus((addr_t)fb_desc));
        if (ret != NO_ERROR)
            return ret;

        uint32_t ack = 0x0;
        ret = mailbox_read(ch_framebuffer, &ack);
        if (ret != NO_ERROR)
            return ret;

        arch_invalidate_cache_range((addr_t)fb_desc,sizeof(fb_mbox_t));

    return ret;
}

status_t init_framebuffer(void) {

    fb_desc.phys_width = 800;
    fb_desc.phys_height = 480;
    fb_desc.virt_width = 800;
    fb_desc.virt_height = 480;
    fb_desc.pitch = 0;
    fb_desc.depth = 32;
    fb_desc.virt_x_offs = 0;
    fb_desc.virt_y_offs = 0;
    fb_desc.fb_p = 0;
    fb_desc.fb_size = 0;

    status_t ret = mailbox_get_framebuffer(&fb_desc);

    return ret;
}

void dispflush(void) {

    //arch_clean_cache_range(fb_desc,sizeof(fb_mbox_t));

}

/* LK display (lib/gfx.h) calls this function */
status_t display_get_framebuffer(struct display_framebuffer *fb)
{
    // VideoCore returns 32-bit bus address, which needs to be converted to kernel virtual
    fb->image.pixels = paddr_to_kvaddr(fb_desc.fb_p & 0x3fffffff);

    fb->format = DISPLAY_FORMAT_ARGB_8888;
    fb->image.format = IMAGE_FORMAT_ARGB_8888;
    fb->image.rowbytes = fb_desc.phys_width * fb_desc.depth/8;

    fb->image.width = fb_desc.phys_width;
    fb->image.height = fb_desc.phys_height;
    fb->image.stride = fb_desc.phys_width;
    fb->flush = NULL;

    return NO_ERROR;
}

status_t display_get_info(struct display_info *info)
{
    info->format = DISPLAY_FORMAT_ARGB_8888;
    info->width = fb_desc.phys_width;
    info->height = fb_desc.phys_height;

    return NO_ERROR;
}

status_t display_present(struct display_image *image, uint starty, uint endy)
{
  return NO_ERROR;
}



