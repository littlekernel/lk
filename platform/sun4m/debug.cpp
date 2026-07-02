//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
//
#include <stdint.h>
#include <platform/debug.h>

#include "platform_p.h"

enum {
    CONTROL_REG = 4,
    DATA_REG = 6
};

// Real physical 36-bit address,
// can access via ASI 0x2f (0x20 + 0xf which is the top 4 bits).
constexpr uint64_t SCC_BASE = 0xf'f110'0000ULL;

void platform_dputc(char c) {
    if (c == '\n') {
        while ((sparc_read_physical_8(SCC_BASE + CONTROL_REG) & 0x04) == 0) {
            // spin
        }
        sparc_write_physical_8(SCC_BASE + DATA_REG, '\r');
    }

    while ((sparc_read_physical_8(SCC_BASE + CONTROL_REG) & 0x04) == 0) {
        // spin
    }
    sparc_write_physical_8(SCC_BASE + DATA_REG, c);
}

int platform_dgetc(char *c, bool wait) {
    if (wait) {
        while ((sparc_read_physical_8(SCC_BASE + CONTROL_REG) & 0x01) == 0) {
            // spin
        }
        *c = sparc_read_physical_8(SCC_BASE + DATA_REG);
        return 0;
    } else {
        if ((sparc_read_physical_8(SCC_BASE + CONTROL_REG) & 0x01) != 0) {
            *c = sparc_read_physical_8(SCC_BASE + DATA_REG);
            return 0;
        }
        return -1;
    }
}

