/*
 * Copyright (c) 2014 Brian Swetland
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

#include <stdint.h>

typedef struct {
    uint32_t kind;
    uint32_t type;
    uint32_t offset;    /* byte offset from start of file */
    uint32_t length;    /* length in bytes */
    uint8_t name[16];
    uint8_t sha256[32];
} __attribute__ ((packed)) bootentry_file;

typedef struct {
    uint32_t kind;
    union {
        uint32_t u[15];
        uint8_t b[60];
    } __attribute__((packed)) u;
} __attribute__((packed)) bootentry_data;

typedef struct {
    uint32_t kind;
    uint32_t version;       /* bootimage version */
    uint32_t image_size;    /* byte size of entire image */
    uint32_t entry_count;   /* number of valid bootentries */
    uint32_t reserved[12];
} __attribute__((packed)) bootentry_info;

typedef union {
    uint32_t kind;
    bootentry_file file;
    bootentry_data data;
    bootentry_info info;
} bootentry;

#define BOOT_VERSION 0x00010000     /* 1.0 */

#define BOOT_MAGIC "<lk-boot-image>"
#define BOOT_MAGIC_LENGTH 16

// header (bootentry_file, but special):

// bootentry kinds:
#define KIND_FILE           0x656c6966  // 'file'
#define KIND_BOOT_INFO      0x6f666e69  // 'info'
#define KIND_BOARD          0x67726174  // 'targ' board id string
#define KIND_BUILD          0x706d7473  // 'stmp' build id string

// bootentry_file types:
#define TYPE_BOOT_IMAGE     0x746f6f62  // 'boot'
#define TYPE_LK             0x6b6c6b6c  // 'lklk'
#define TYPE_FPGA_IMAGE     0x61677066  // 'fpga'
#define TYPE_LINUX_KERNEL   0x6b78636c  // 'lcxk'
#define TYPE_LINUX_INITRD   0x64697264  // 'drid'
#define TYPE_DEVICE_TREE    0x74766564  // 'devt'
#define TYPE_SYSPARAMS      0x70737973  // 'sysp'
#define TYPE_UNKNOWN        0x6e6b6e75  // 'unkn'

// first entry must be:
//   kind: KIND_FILE
//   type: TYPE_BOOT_IMAGE
//   offset: 0
//   length: 4096
//   name: BOOT_MAGIC
//   sha256: of bootentry[1..63]

// second entry must be:
//   kind: KIND_BOOT_INFO

// offsets should be multiple-of-4096
