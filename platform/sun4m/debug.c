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
#define SCC_BASE (0x0000000ff1100000ULL)

void platform_dputc(char c) {

    if (c == '\n') {
        while ((read_control_space_8(SCC_BASE, CONTROL_REG) & 0x04) == 0) {
            // spin
        }
        write_control_space_8(SCC_BASE, DATA_REG, '\r');
    }

    while ((read_control_space_8(SCC_BASE, CONTROL_REG) & 0x04) == 0) {
        // spin
    }
    write_control_space_8(SCC_BASE, DATA_REG, c);
}

int platform_dgetc(char *c, bool wait) {
    if (wait) {
        while ((read_control_space_8(SCC_BASE, CONTROL_REG) & 0x01) == 0) {
            // spin
        }
        *c = read_control_space_8(SCC_BASE, DATA_REG);
        return 0;
    } else {
        if ((read_control_space_8(SCC_BASE, CONTROL_REG) & 0x01) != 0) {
            *c = read_control_space_8(SCC_BASE, DATA_REG);
            return 0;
        }
        return -1;
    }
}

