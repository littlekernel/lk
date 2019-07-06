/*
 * Copyright (c) 2013 Heather Lee Wilson
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <norfs_test_helper.h>
#include <lib/norfs.h>
#include <dev/flash_nor.h>
#include <platform/flash_nor_config.h>
#include <lk/debug.h>

void dump_bank(void) {
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

void wipe_fs(void) {
    norfs_unmount_fs();
    flash_nor_begin(0);
    flash_nor_erase_pages(0, 0 + norfs_nvram_offset, 8 * FLASH_PAGE_SIZE);
    flash_nor_end(0);
}
