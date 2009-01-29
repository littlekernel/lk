/*
 * Copyright (c) 2008, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the 
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED 
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef __PLATFORM_MSM7K_MDDI_HW_H
#define __PLATFORM_MSM7K_MDDI_HW_H

#define MSM_MDDI_BASE 0xAA600000

/* see 80-VA736-2 C pp 776-787 */

#define MDDI_REG(off) (MSM_MDDI_BASE + (off))

#define MDDI_CMD               MDDI_REG(0x0000)
#define MDDI_VERSION           MDDI_REG(0x0004)
#define MDDI_PRI_PTR           MDDI_REG(0x0008)
#define MDDI_SEC_PTR           MDDI_REG(0x000C)
#define MDDI_BPS               MDDI_REG(0x0010)
#define MDDI_SPM               MDDI_REG(0x0014)
#define MDDI_INT               MDDI_REG(0x0018)

#define MDDI_INT_PRI_PTR_READ          (1 << 0)
#define MDDI_INT_SEC_PTR_READ          (1 << 1)
#define MDDI_INT_REV_DATA_AVAIL        (1 << 2)
#define MDDI_INT_DISP_REQ              (1 << 3)
#define MDDI_INT_PRI_UNDERFLOW         (1 << 4)
#define MDDI_INT_SEC_UNDERFLOW         (1 << 5)
#define MDDI_INT_REV_OVERFLOW          (1 << 6)
#define MDDI_INT_CRC_ERROR             (1 << 7)
#define MDDI_INT_MDDI_IN               (1 << 8)
#define MDDI_INT_PRI_OVERWRITE         (1 << 9)
#define MDDI_INT_SEC_OVERWRITE         (1 << 10)
#define MDDI_INT_REV_OVERWRITE         (1 << 11)
#define MDDI_INT_DMA_FAILURE           (1 << 12)
#define MDDI_INT_LINK_ACTIVE           (1 << 13)
#define MDDI_INT_IN_HIBERNATION        (1 << 14)
#define MDDI_INT_PRI_LINK_LIST_DONE    (1 << 15)
#define MDDI_INT_SEC_LINK_LIST_DONE    (1 << 16)
#define MDDI_INT_NO_REQ_PKTS_PENDING   (1 << 17)
#define MDDI_INT_RTD_FAILURE           (1 << 18)
#define MDDI_INT_REV_PKT_RECEIVED      (1 << 19)
#define MDDI_INT_REV_PKTS_AVAIL        (1 << 20)

#define MDDI_INTEN             MDDI_REG(0x001C)
#define MDDI_REV_PTR           MDDI_REG(0x0020)
#define MDDI_REV_SIZE          MDDI_REG(0x0024)
#define MDDI_STAT              MDDI_REG(0x0028)

#define MDDI_STAT_LINK_ACTIVE                (1 << 0)
#define MDDI_STAT_NEW_REV_PTR                (1 << 1)
#define MDDI_STAT_NEW_PRI_PTR                (1 << 2)
#define MDDI_STAT_NEW_SEC_PTR                (1 << 3)
#define MDDI_STAT_IN_HIBERNATION             (1 << 4)
#define MDDI_STAT_PRI_LINK_LIST_DONE         (1 << 5)
#define MDDI_STAT_SEC_LINK_LIST_DONE         (1 << 6)
#define MDDI_STAT_SEND_TIMING_PKT            (1 << 7)
#define MDDI_STAT_SEND_REV_ENCAP_WITH_FLAGS  (1 << 8)
#define MDDI_STAT_SEND_POWER_DOWN            (1 << 9)
#define MDDI_STAT_DO_HANDSHAKE               (1 << 10)
#define MDDI_STAT_RTD_MEAS_FAIL              (1 << 11)
#define MDDI_STAT_CLIENT_WAKEUP_REQ          (1 << 12)
#define MDDI_STAT_DMA_ABORT                  (1 << 13)
#define MDDI_STAT_REV_OVERFLOW_RESET         (1 << 14)
#define MDDI_STAT_FORCE_NEW_REV_PTR          (1 << 15)
#define MDDI_STAT_CRC_ERRORS                 (1 << 16)

#define MDDI_REV_RATE_DIV      MDDI_REG(0x002C)
#define MDDI_REV_CRC_ERR       MDDI_REG(0x0030)
#define MDDI_TA1_LEN           MDDI_REG(0x0034)
#define MDDI_TA2_LEN           MDDI_REG(0x0038)
#define MDDI_TEST_BUS          MDDI_REG(0x003C)
#define MDDI_TEST              MDDI_REG(0x0040)
#define MDDI_REV_PKT_CNT       MDDI_REG(0x0044)
#define MDDI_DRIVE_HI          MDDI_REG(0x0048)
#define MDDI_DRIVE_LO          MDDI_REG(0x004C)
#define MDDI_DISP_WAKE         MDDI_REG(0x0050)
#define MDDI_REV_ENCAP_SZ      MDDI_REG(0x0054)
#define MDDI_RTD_VAL           MDDI_REG(0x0058)
#define MDDI_MDP_VID_FMT_DES   MDDI_REG(0x005C)
#define MDDI_MDP_VID_PIX_ATTR  MDDI_REG(0x0060)
#define MDDI_MDP_VID_CLIENTID  MDDI_REG(0x0064)
#define MDDI_PAD_CTL           MDDI_REG(0x0068)
#define MDDI_DRIVER_START_CNT  MDDI_REG(0x006C)
#define MDDI_NEXT_PRI_PTR      MDDI_REG(0x0070)
#define MDDI_NEXT_SEC_PTR      MDDI_REG(0x0074)
#define MDDI_MISR_CTL          MDDI_REG(0x0078)
#define MDDI_MISR_DATA         MDDI_REG(0x007C)
#define MDDI_SF_CNT            MDDI_REG(0x0080)
#define MDDI_MF_CNT            MDDI_REG(0x0084)
#define MDDI_CURR_REV_PTR      MDDI_REG(0x0088)
#define MDDI_CORE_VER          MDDI_REG(0x008C)

#define CMD_POWER_DOWN         0x0100
#define CMD_POWER_UP           0x0200
#define CMD_HIBERNATE          0x0300
#define CMD_RESET              0x0400
#define CMD_IGNORE             0x0501
#define CMD_LISTEN             0x0500
#define CMD_REV_ENC_REQ        0x0600
#define CMD_RTD_MEASURE        0x0700
#define CMD_LINK_ACTIVE        0x0900
#define CMD_PERIODIC_REV_ENC   0x0A00
#define CMD_FORCE_NEW_REV_PTR  0x0C00

#define CMD_GET_CLIENT_CAP     0x0601
#define CMD_GET_CLIENT_STATUS  0x0602

#if 1
#define FORMAT_18BPP           0x5666
#define FORMAT_24BPP           0x5888
#define FORMAT_16BPP           0x5565
#else
#define FORMAT_MONOCHROME      (0 << 13)
#define FORMAT_PALETTE         (1 << 13)
#define FORMAT_RGB             (2 << 13)
#define FORMAT_YCBCR422        (3 << 13)
#define FORMAT_BAYER           (4 << 13)
#endif

#define PIXATTR_BOTH_EYES      3
#define PIXATTR_LEFT_EYE       2
#define PIXATTR_RIGHT_EYE      1
#define PIXATTR_ALT_DISPLAY    0

#define PIXATTR_PROGRESSIVE    0
#define PIXATTR_INTERLACED     (1 << 2)
#define PIXATTR_ALTERNATE      (1 << 3)

#define PIXATTR_IGNORE_LRTB    (1 << 5)

#define PIXATTR_TO_REFRESH     (0 << 6)
#define PIXATTR_TO_OFFLINE     (1 << 6)
#define PIXATTR_TO_ALL         (3 << 6)

#define PIXATTR_LAST_ROW       (1 << 15)

#define TYPE_VIDEO_STREAM      16
#define TYPE_CLIENT_CAPS       66
#define TYPE_REGISTER_ACCESS   146
#define TYPE_CLIENT_STATUS     70

typedef struct mddi_video_stream mddi_video_stream;
typedef struct mddi_register_access mddi_register_access;
typedef struct mddi_client_caps mddi_client_caps;

typedef struct mddi_llentry mddi_llentry;

struct __attribute__((packed)) mddi_video_stream 
{
    unsigned short length;      /* length in bytes excluding this field */
    unsigned short type;        /* MDDI_TYPE_VIDEO_STREAM */
    unsigned short client_id;   /* set to zero */
    
    unsigned short format;
    unsigned short pixattr;

    unsigned short left;
    unsigned short top;
    unsigned short right;
    unsigned short bottom;

    unsigned short start_x;
    unsigned short start_y;

    unsigned short pixels;

    unsigned short crc;
    unsigned short reserved;
};

struct __attribute__((packed)) mddi_register_access
{
    unsigned short length;
    unsigned short type;
    unsigned short client_id;

    unsigned short rw_info;    /* flag below | count of reg_data */
#define MDDI_WRITE     (0 << 14)
#define MDDI_READ      (2 << 14)
#define MDDI_READ_RESP (3 << 14)
    
    unsigned reg_addr;
    unsigned short crc;        /* 16 bit crc of the above */

    unsigned reg_data;         /* "list" of 3byte data values */
};

struct __attribute__((packed)) mddi_llentry {
    unsigned short flags;
    unsigned short header_count;
    unsigned short data_count;
    void *data;
    mddi_llentry *next;
    unsigned short reserved;
    union {
        mddi_video_stream v;
        mddi_register_access r;
        unsigned _[12];
    } u;
};

#endif /* __PLATFORM_MSM7K_MDDI_HW_H */
