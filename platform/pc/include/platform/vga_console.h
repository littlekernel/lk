/*
 * Copyright (c) 2009 Corey Tabaka
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <stdbool.h>
#include <lk/compiler.h>

// Simple interface for the VGA text mode console.

__BEGIN_CDECLS

void vga_console_init(void);
bool vga_console_present(void);
void vga_console_putc(char c);

__END_CDECLS
