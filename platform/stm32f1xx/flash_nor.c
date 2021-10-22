/*
 * Copyright (c) 2012 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/debug.h>
#include <lk/trace.h>
#include <lk/err.h>
#include <sys/types.h>
#include <kernel/thread.h>
#include <platform.h>
#include <platform/stm32.h>
#include <dev/flash_nor.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_flash.h>
#include <misc.h>

/* flash size and page size determined dynamically */
#define FLASH_SIZE ((*(REG16(0x1FFFF7E0))) * (size_t)1024)
#define FLASH_PAGE_SIZE ((size_t)((FLASH_SIZE > 131072) ? 2048 : 1024))

struct flash_nor_bank flash[1];

void stm32_flash_nor_early_init(void) {
    FLASH_Lock(); // make sure it's locked

    flash[0].base = 0x08000000;
    flash[0].len = FLASH_SIZE;
    flash[0].page_size = FLASH_PAGE_SIZE;
    flash[0].flags = 0;
}

void stm32_flash_nor_init(void) {
    TRACEF("flash size %zu\n", FLASH_SIZE);
    TRACEF("page size %zu\n", FLASH_PAGE_SIZE);
}

const struct flash_nor_bank *flash_nor_get_bank(unsigned int bank) {
    if (bank != 0)
        return NULL;

    return &flash[0];
}

