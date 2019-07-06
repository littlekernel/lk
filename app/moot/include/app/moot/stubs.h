/*
 * Copyright 2016 Google Inc. All Rights Reserved.
 * Author: gkalsi@google.com (Gurjant Kalsi)
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#pragma once

#include <sys/types.h>

typedef struct moot_sysinfo {
    uintptr_t sys_base_addr;  // Pointer to the base of the main system image.

    size_t btldr_offset;
    size_t bootloader_len;

    size_t system_offset;
    size_t system_len;

    char *system_flash_name;
} moot_sysinfo_t;

// Must be implemented by the platform;
extern const moot_sysinfo_t moot_system_info;

// Returns NO_ERROR if it was successfully able to mount a secondary flash
// device. If NO_ERROR is returned, mount_path and device_name should also be
// populated to reflect the path at which the FS was mounted and the name of
// the BIO device that hosts the FS.
status_t moot_mount_default_fs(char **mount_path, char **device_name);
