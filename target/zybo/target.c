/*
 * Copyright (c) 2014 Travis Geiselbrecht
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
#include <stdio.h>
#include <dev/spiflash.h>
#include <lib/ptable.h>
#include <lib/sysparam.h>

void target_early_init(void)
{
}

void target_init(void)
{
    /* zybo has a spiflash on qspi */
    spiflash_detect();

    bdev_t *spi = bio_open("spi0");

#if WITH_LIB_PTABLE && WITH_LIB_SYSPARAM
    if (spi) {
        /* find or create a partition table at the start of flash */
        if (ptable_scan(spi, 0) < 0) {
            ptable_create_default(spi, 0);
        }

        struct ptable_entry entry = { 0 };

        if (ptable_find("sysparam", &entry) < 0) {
            /* didn't find sysparam partition, create it */
            ptable_add("sysparam", 0x1000, 0x1000, 0);
            ptable_find("sysparam", &entry);
        }

        printf("flash partition table:\n");
        ptable_dump();

        if (entry.length > 0) {
            sysparam_scan(spi, entry.offset, entry.length);

#if SYSPARAM_ALLOW_WRITE
            /* for testing purposes, put at least one sysparam value in */
            if (sysparam_add("dummy", "value", sizeof("value")) >= 0) {
                sysparam_write();
            }
#endif

            sysparam_dump(true);
        }
    }
#endif
}

