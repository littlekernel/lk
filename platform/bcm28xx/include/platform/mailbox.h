/*
 * Copyright (c) 2017 Eric Holland
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

#include <platform/bcm28xx.h>


#define VCORE_MAILBOX_PHYSICAL_ADDRESS (ARMCTRL_0_SBM_BASE + 0x80)


#define VCORE_TAG_REQUEST 0x00000000
#define VCORE_ENDTAG	  0x00000000

#define VCORE_TAG_GET_FIRMWARE_REV			0x00000001
#define VCORE_TAG_GET_FIRMWARE_REV_REQ_LEN	0x00000000
#define VCORE_TAG_GET_FIRMWARE_REV_RSP_LEN	0x00000004

#define VCORE_MAILBOX_FULL	0x80000000
#define VCORE_MAILBOX_EMPTY	0x40000000

#define VC_FB_CHANNEL 0x01
#define ARM_TO_VC_CHANNEL 0x08
#define VC_TO_ARM_CHANNEL 0x09

#define VCORE_SUCCESS			0
#define VCORE_ERR_MBOX_FULL 	1
#define VCORE_ERR_MBOX_TIMEOUT 	2

#define VCORE_READ_ATTEMPTS 0xffffffff


#define MAILBOX_READ               0
#define MAILBOX_PEEK               2
#define MAILBOX_CONDIG             4
#define MAILBOX_STATUS             6
#define MAILBOX_WRITE              8


#define MAILBOX_FULL               0x80000000
#define MAILBOX_EMPTY              0x40000000

#define MAX_MAILBOX_READ_ATTEMPTS  8

enum mailbox_channel {
    ch_power               = 0,
    ch_framebuffer         = 1,
    ch_vuart               = 2,
    ch_vchic               = 3,
    ch_leds                = 4,
    ch_buttons             = 5,
    ch_touchscreen         = 6,
    ch_unused              = 7,
    ch_propertytags_tovc   = 8,
    ch_propertytags_fromvc = 9,
};

typedef struct {
	uint32_t phys_width;	//request
	uint32_t phys_height;	//request
	uint32_t virt_width;	//request
	uint32_t virt_height;	//request
	uint32_t pitch;			//response
	uint32_t depth;			//request
	uint32_t virt_x_offs;	//request
	uint32_t virt_y_offs;	//request
	uint32_t fb_p;			//response
	uint32_t fb_size;		//response
} fb_mbox_t;

uint32_t get_vcore_framebuffer(fb_mbox_t * fb_mbox);
status_t init_framebuffer(void);
uint32_t _get_vcore_single(uint32_t tag, uint32_t req_len, uint8_t * rsp, uint32_t rsp_len);

