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

#ifndef __PLATFORM_MSM_SHARED_NAND_H
#define __PLATFORM_MSM_SHARED_NAND_H

#define MSM_NAND_BASE 0xA0A00000

/* see 80-VA736-2 C pp 354-414 */

#define NAND_REG(off) (MSM_NAND_BASE + (off))

#define NAND_FLASH_CMD            NAND_REG(0x0000)
#define NAND_ADDR0                NAND_REG(0x0004)
#define NAND_ADDR1                NAND_REG(0x0008)
#define NAND_FLASH_CHIP_SELECT    NAND_REG(0x000C)
#define NAND_EXEC_CMD             NAND_REG(0x0010)
#define NAND_FLASH_STATUS         NAND_REG(0x0014)
#define NAND_BUFFER_STATUS        NAND_REG(0x0018)
#define NAND_DEV0_CFG0            NAND_REG(0x0020)
#define NAND_DEV0_CFG1            NAND_REG(0x0024)
#define NAND_DEV1_CFG0            NAND_REG(0x0030)
#define NAND_DEV1_CFG1            NAND_REG(0x0034)
#define NAND_READ_ID              NAND_REG(0x0040)
#define NAND_READ_STATUS          NAND_REG(0x0044)
#define NAND_CONFIG_DATA          NAND_REG(0x0050)
#define NAND_CONFIG               NAND_REG(0x0054)
#define NAND_CONFIG_MODE          NAND_REG(0x0058)
#define NAND_CONFIG_STATUS        NAND_REG(0x0060)
#define NAND_MACRO1_REG           NAND_REG(0x0064)
#define NAND_XFR_STEP1            NAND_REG(0x0070)
#define NAND_XFR_STEP2            NAND_REG(0x0074)
#define NAND_XFR_STEP3            NAND_REG(0x0078)
#define NAND_XFR_STEP4            NAND_REG(0x007C)
#define NAND_XFR_STEP5            NAND_REG(0x0080)
#define NAND_XFR_STEP6            NAND_REG(0x0084)
#define NAND_XFR_STEP7            NAND_REG(0x0088)
#define NAND_DEV_CMD0             NAND_REG(0x00A0)
#define NAND_DEV_CMD1             NAND_REG(0x00A4)
#define NAND_DEV_CMD2             NAND_REG(0x00A8)
#define NAND_DEV_CMD_VLD          NAND_REG(0x00AC)
#define NAND_EBI2_MISR_SIG_REG    NAND_REG(0x00B0)
#define NAND_EBI2_ECC_BUF_CFG     NAND_REG(0x00F0)
#define NAND_FLASH_BUFFER         NAND_REG(0x0100)

/* device commands */

#define NAND_CMD_SOFT_RESET         0x01
#define NAND_CMD_PAGE_READ          0x32
#define NAND_CMD_PAGE_READ_ECC      0x33
#define NAND_CMD_PAGE_READ_ALL      0x34
#define NAND_CMD_SEQ_PAGE_READ      0x15
#define NAND_CMD_PRG_PAGE           0x36
#define NAND_CMD_PRG_PAGE_ECC       0x37
#define NAND_CMD_PRG_PAGE_ALL       0x39
#define NAND_CMD_BLOCK_ERASE        0x3A
#define NAND_CMD_FETCH_ID           0x0B
#define NAND_CMD_STATUS             0x0C
#define NAND_CMD_RESET              0x0D

#endif /* __PLATFORM_MSM_SHARED_NAND_H */
