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

struct platform_ide_config {
    bool isa;
    uint32_t channel;
    uint16_t io_base;
    uint16_t ctrl_base;
    int irq;
};

status_t platform_ide_init(const struct platform_ide_config *config);
