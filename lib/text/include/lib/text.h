/*
 * Copyright (c) 2010 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lib/font.h>

/* super cheezy mechanism to stick lines of text up on top of whatever is being drawn */
/* XXX replace with something more generic later */
void text_draw(int x, int y, const char *string);

/* super dumb, someone has to call this to refresh everything */
void text_update(void);

