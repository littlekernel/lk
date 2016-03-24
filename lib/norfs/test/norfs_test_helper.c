/*
 * Copyright (c) 2013 Heather Lee Wilson
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
#include <norfs_test_helper.h>
#include <lib/norfs.h>
#include <dev/flash_nor.h>
#include <platform/flash_nor_config.h>
#include <debug.h>

void dump_bank(void)
{
    const struct flash_nor_bank *bank;
    bank = flash_nor_get_bank(0);
    printf("\n\n**DUMP BANK**\n");
    uint8_t *addr = (uint8_t *)bank->base;
    for (int i = 0; i < 8 * FLASH_PAGE_SIZE; i++) {
        if (i%4 == 0) printf("  ");
        if (i%FLASH_PAGE_SIZE == 0)
            printf("\n\n\n\n\n\ni: %d\n", i);
        printf("%02X ", *(addr + i));
    }
}

void wipe_fs(void)
{
    norfs_unmount_fs();
    flash_nor_begin(0);
    flash_nor_erase_pages(0, 0 + norfs_nvram_offset, 8 * FLASH_PAGE_SIZE);
    flash_nor_end(0);
}
