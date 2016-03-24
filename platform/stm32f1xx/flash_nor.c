/*
 * Copyright (c) 2012 Travis Geiselbrecht
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
#include <debug.h>
#include <trace.h>
#include <err.h>
#include <sys/types.h>
#include <kernel/thread.h>
#include <platform.h>
#include <dev/flash_nor.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_flash.h>
#include <misc.h>

/* flash size and page size determined dynamically */
#define FLASH_SIZE ((*(REG16(0x1FFFF7E0))) * (size_t)1024)
#define FLASH_PAGE_SIZE ((size_t)((FLASH_SIZE > 131072) ? 2048 : 1024))

struct flash_nor_bank flash[1];

void stm32_flash_nor_early_init(void)
{
    FLASH_Lock(); // make sure it's locked

    flash[0].base = 0x08000000;
    flash[0].len = FLASH_SIZE;
    flash[0].page_size = FLASH_PAGE_SIZE;
    flash[0].flags = 0;
}

void stm32_flash_nor_init(void)
{
    TRACEF("flash size %zu\n", FLASH_SIZE);
    TRACEF("page size %zu\n", FLASH_PAGE_SIZE);
}

const struct flash_nor_bank *flash_nor_get_bank(unsigned int bank)
{
    if (bank != 0)
        return NULL;

    return &flash[0];
}

