/*
 * Copyright (c) 2026 Josh Cummings
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include "platform_p.h"
#include <dev/display.h>
#include <lib/page_alloc.h>
#include <lk/err.h>
#include <lk/main.h>
#include <platform/display.h>
#include <string.h>
#if WITH_KERNEL_VM
#include <kernel/vm.h>
#else
#include <kernel/novm.h>
#endif

static volatile uint16_t *const paula_base = (void *)0xDFF000;
static struct display_framebuffer display_fb;

// 1bpp bitplane
static uint8_t bitplane[BPL_BYTES];
static uint16_t copper[64];

void make_copper_list(uint8_t *bpl) {
    // Bitplane ptr register is two 16bit halves. May need to change this if vm/MMU is ever enabled.
    uintptr_t bpl_ptr = (uintptr_t)bpl;
    uint16_t bplptr_hi = (bpl_ptr >> 16);
    uint16_t bplptr_lo = (bpl_ptr & 0xFFFF);

    int i = 0;

    // clang-format off
   
   // Copper instructions are two words. First word is the destination register offset, second is the value.
   
   // Set display window
   copper[i++] = DIWSTRT; copper[i++] = 0x2C81; // Display window vert + horiz start, PAL low-res mode
   copper[i++] = DIWSTOP; copper[i++] = ((((0x2C + H) & 0xFF) << 8) | 0xC1);  // vert + horiz end
   copper[i++] = DDFSTRT; copper[i++] = 0x0038; // low-res DMA fetch start
   copper[i++] = DDFSTOP; copper[i++] = 0x00D0; // DMA fetch stop. Stop and start may need to change if screen width ever does.

   // 1 bitplane, enable "colour" output
   copper[i++] = BPLCON0; copper[i++] = (0x200 | (1 << 12)); // Set display mode and bitplane count ("BPU"); one bitplane
   copper[i++] = BPLCON1; copper[i++] = 0x0000; // No horizontal playfield scroll
   copper[i++] = BPLCON2; copper[i++] = 0x0000; // Set playfield priority

   // Bitplane modulo = 0 for a single linear bitplane 
   copper[i++] = BPL1MOD; copper[i++] = 0x0000;
   copper[i++] = BPL2MOD; copper[i++] = 0x0000;

   // Set bitplane pointer
   copper[i++] = BPL1PTH; copper[i++] = bplptr_hi;
   copper[i++] = BPL1PTL; copper[i++] = bplptr_lo;

   // Background and foreground colours
   copper[i++] = COLOR00; copper[i++] = 0x0000;
   copper[i++] = COLOR01; copper[i++] = 0x0FFF;

   // Send end/terminator words, causes Copper to stop processing the list
   copper[i++] = 0xFFFF;
   copper[i++] = 0xFFFE;

    // clang-format on
}

static void write_reg(unsigned int reg, uint32_t val) {
    paula_base[reg >> 1] = val;
}

status_t display_get_framebuffer(struct display_framebuffer *fb) {
    fb->image.pixels = bitplane;

    fb->image.format = IMAGE_FORMAT_MONO_1;
    fb->image.rowbytes = BYTES_PER_ROW;
    fb->image.width = W;
    fb->image.height = H;
    fb->image.stride = BYTES_PER_ROW;
    fb->format = DISPLAY_FORMAT_MONO_1;
    fb->flush = NULL;

    return NO_ERROR;
}

status_t display_get_info(struct display_info *info) {
    info->format = DISPLAY_FORMAT_MONO_1;
    info->width = W;
    info->height = H;

    return NO_ERROR;
}

void platform_init_display(void) {
    memset(bitplane, 0, BPL_BYTES);

    make_copper_list(bitplane);

    // Disable DMA
    write_reg(DMACON, 0x7FFF);

    // Set up copper list address, high and low.
    // Physical address, might need revisiting if/when vm/MMU
    uintptr_t clist = (uintptr_t)copper;
    write_reg(COP1LCH, (clist >> 16));
    write_reg(COP1LCL, (clist & 0xFFFF));
    write_reg(COPJMP1, 0x0000);

    // Enable DMA.
    // 0x200 = master DMA enable, 0x100 = bitplane DMA enable, 0x0080 = Copper DMA enable
    write_reg(DMACON, (0x8000 | 0x0200 | 0x0080 | 0x0100));

    display_get_framebuffer(&display_fb);
}
