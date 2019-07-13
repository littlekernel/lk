/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <sys/types.h>
#include <lk/compiler.h>

/* GLOBAL STATUS VALUES */
#define STD_COMMAND                 0x00
#define SETUP_COMMAND_PHASE         0x40
#define FUNCTION_ERROR              0x7F    /* Used when we are stalling the function EP0 */
#define HUB_ERROR                   0xFF    /* Used when we are stalling the HUB EP0 */

/* Request Types */
#define DIR_OUT                     (0 << 7)
#define DIR_IN                      (1 << 7)
#define DIR_MASK                    (1 << 7)
#define TYPE_STANDARD               (0 << 5)
#define TYPE_CLASS                  (1 << 5)
#define TYPE_VENDOR                 (2 << 5)
#define TYPE_MASK                   (3 << 5)
#define RECIP_DEVICE                (0 << 0)
#define RECIP_INTERFACE             (1 << 0)
#define RECIP_ENDPOINT              (2 << 0)
#define RECIP_OTHER                 (3 << 0)
#define RECIP_MASK                  (0x1f << 0)

/* 1.0 Request Values */
#define GET_STATUS                  0x00
#define CLEAR_FEATURE               0x01
#define SET_FEATURE                 0x03
#define SET_ADDRESS                 0x05
#define GET_DESCRIPTOR              0x06
#define SET_DESCRIPTOR              0x07
#define GET_CONFIGURATION           0x08
#define SET_CONFIGURATION           0x09
#define GET_INTERFACE               0x0A
#define SET_INTERFACE               0x0B
#define SYNCH_FRAME                 0x0C

/* Mass storage requests */
#define MASS_STORAGE_GET_MAX_LUN    0xfe
#define MASS_STORAGE_RESET          0xff

/* DFU requests */
#define DFU_DETACH                  0x00
#define DFU_DNLOAD                  0x01
#define DFU_UPLOAD                  0x02
#define DFU_GETSTATUS               0x03
#define DFU_CLRSTATUS               0x04
#define DFU_GETSTATE                0x05
#define DFU_ABORT                   0x06

/* HID Request Values */
#define GET_REPORT                  0x01
#define GET_IDLE                    0x02
#define GET_PROTOCOL                0x03
#define SET_REPORT                  0x09
#define SET_IDLE                    0x0A
#define SET_PROTOCOL                0x0B

/* Descriptor Types */
#define DEVICE                      0x01
#define CONFIGURATION               0x02
#define STRING                      0x03
#define INTERFACE                   0x04
#define ENDPOINT                    0x05
#define DEVICE_QUALIFIER            0x06
#define OTHER_SPEED_CONFIGURATION   0x07
#define INTERFACE_POWER             0x08
#define HID                         0x21
#define HIDREPORT                   0x22
#define HIDPHYSICAL                 0x23

/* general USB defines */
struct usb_setup {
    uint8_t request_type;
    uint8_t request;
    uint16_t value;
    uint16_t index;
    uint16_t length;
} __PACKED;
