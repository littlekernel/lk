/*
 * Copyright (c) 2008-2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>
#include <sys/types.h>

__BEGIN_CDECLS

int rand(void);
void srand(unsigned int seed);

/* non standard extension to add some entropy to the seed */
void rand_add_entropy(const void *buf, size_t len);

__END_CDECLS

