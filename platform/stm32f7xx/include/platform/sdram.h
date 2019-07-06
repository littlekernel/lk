/*
 * Copyright (c) 2015 Carlos Pizano-Uribe <cpu@chromium.org>
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
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

