/*
 * Copyright (c) 2008-2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <rand.h>
#include <sys/types.h>

static unsigned int randseed = 12345;

void srand(unsigned int seed) {
    randseed = seed;
}

void rand_add_entropy(const void *buf, size_t len) {
    if (len == 0)
        return;

    uint32_t enp = 0;
    for (size_t i = 0; i < len; i++) {
        uint32_t c = ((const uint8_t *)buf)[i];
        enp = ((enp << 8) | (enp >> 24)) ^ c;
    }

    randseed ^= enp;
}

int rand(void) {
    return (randseed = randseed * 1664525 + 1013904223);
}
