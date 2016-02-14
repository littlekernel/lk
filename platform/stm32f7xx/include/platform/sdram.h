/*
 * Copyright (c) 2015 Carlos Pizano-Uribe <cpu@chromium.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#pragma once

#include <stdint.h>

enum sdram_bus_width {
    SDRAM_BUS_WIDTH_8,
    SDRAM_BUS_WIDTH_16,
    SDRAM_BUS_WIDTH_32
};

enum sdram_cas_latency {
    SDRAM_CAS_LATENCY_1,
    SDRAM_CAS_LATENCY_2,
    SDRAM_CAS_LATENCY_3
};

enum sdram_col_bits_num {
    SDRAM_COLUMN_BITS_8,
    SDRAM_COLUMN_BITS_9,
    SDRAM_COLUMN_BITS_10,
    SDRAM_COLUMN_BITS_11
};

typedef struct _sdram_config {
    enum sdram_bus_width bus_width;
    enum sdram_cas_latency cas_latency;
    enum sdram_col_bits_num col_bits_num;
} sdram_config_t;

// Left to each target to define the GPIO to DRAM bus mapping.
void stm_sdram_GPIO_init(void);

uint8_t stm32_sdram_init(sdram_config_t *config);

