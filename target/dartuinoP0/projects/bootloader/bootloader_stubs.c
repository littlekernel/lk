/*
 * Copyright 2016 Google Inc. All Rights Reserved.
 * Author: gkalsi@google.com (Gurjant Kalsi)
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

static char bootloader_primary_flash_name[] = "flash0";
static char bootloader_secondary_flash_name[] = "qspi-flash";
static char bootloader_mount_point[] = "/spifs";

#include <lib/fs.h>
#include <lk/err.h>
#include <app/moot/stubs.h>
#include <stdio.h>

#define BOOTLOADER_SIZE_KB (64)
#define SYSTEM_FLASH_SIZE_KB (1024)

status_t moot_mount_default_fs(char **mount_path, char **device_name) {
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