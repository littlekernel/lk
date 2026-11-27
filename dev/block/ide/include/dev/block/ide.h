/*
 * Copyright (c) 2013 Corey Tabaka
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <lk/err.h>

// One ISA (or compatibility mode) ATA channel: the command block at io_base, the control
// block at ctrl_base. platform_ide_init() returns ERR_NOT_FOUND if nothing answers there,
// and registers each disk found with bio as ide<channel>.<drive>.
struct platform_ide_config {
    uint32_t channel;
    uint16_t io_base;
    uint16_t ctrl_base;
    int irq;
};

status_t platform_ide_init(const struct platform_ide_config *config);
