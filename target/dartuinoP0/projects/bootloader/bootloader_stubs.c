/*
 * Copyright 2016 Google Inc. All Rights Reserved.
 * Author: gkalsi@google.com (Gurjant Kalsi)
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

static char bootloader_primary_flash_name[] = "flash0";
static char bootloader_secondary_flash_name[] = "qspi-flash";
static char bootloader_mount_point[] = "/spifs";

#include <lib/fs.h>
#include <err.h>
#include <app/moot/stubs.h>
#include <stdio.h>

#define BOOTLOADER_SIZE_KB (64)
#define SYSTEM_FLASH_SIZE_KB (1024)

status_t moot_mount_default_fs(char **mount_path, char **device_name)
{
    *mount_path = bootloader_mount_point;
    *device_name = bootloader_secondary_flash_name;
    return NO_ERROR;
}

const moot_sysinfo_t moot_system_info = {
    .sys_base_addr = 0x00210000,
    .btldr_offset = 0x0,
    .bootloader_len = 1024 * BOOTLOADER_SIZE_KB,
    .system_offset = 1024 * BOOTLOADER_SIZE_KB,
    .system_len = (1024 * (SYSTEM_FLASH_SIZE_KB - BOOTLOADER_SIZE_KB)),
    .system_flash_name = bootloader_primary_flash_name,
};