/*
 * Copyright (c) 2012 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#ifndef __DEV_FLASH_NOR_H
#define __DEV_FLASH_NOR_H

#include <sys/types.h>

struct flash_nor_bank {
    addr_t base;
    size_t len;
    size_t page_size;
    uint flags;
};

#define FLASH_NOR_FLAG_NONE 0

const struct flash_nor_bank *flash_nor_get_bank(unsigned int bank);

#endif

