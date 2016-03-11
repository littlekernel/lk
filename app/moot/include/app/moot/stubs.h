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
