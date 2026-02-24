/*
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>
#include <hw/multiboot.h>

// Interface for a simple frame buffer console, used when multiboot framebuffer info is present.

__BEGIN_CDECLS

void platform_init_display(struct multiboot2_tag_framebuffer *framebuffer);
bool has_display(void);
void dputc(char c);

__END_CDECLS
