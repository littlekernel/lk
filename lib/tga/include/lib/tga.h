/*
 * Copyright (c) 2010 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lib/gfx.h>
#include <sys/types.h>

gfx_surface *tga_decode(const void *ptr, size_t len, gfx_format format);

