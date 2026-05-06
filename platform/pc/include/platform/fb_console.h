/*
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <hw/multiboot.h>
#include <lk/compiler.h>

// Interface for a simple frame buffer console, used when multiboot framebuffer info is present.

__BEGIN_CDECLS

void fb_console_init(struct multiboot2_tag_framebuffer *framebuffer);
bool fb_console_present(void);
void fb_console_dputc(char c);

__END_CDECLS
