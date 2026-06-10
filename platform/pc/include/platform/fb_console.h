/*
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <stdint.h>
#include <lk/compiler.h>

// Interface for a simple frame buffer console, used when multiboot framebuffer info is present.

__BEGIN_CDECLS

struct fb_console_boot_info {
	uint64_t framebuffer_addr;
	uint32_t framebuffer_pitch;
	uint32_t framebuffer_width;
	uint32_t framebuffer_height;
	uint8_t framebuffer_bpp;
};

void fb_console_init(const struct fb_console_boot_info *boot_info);
void fb_console_init_postvm(void);
bool fb_console_present(void);
void fb_console_dputc(char c);

__END_CDECLS
