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
#pragma once

#include <reg.h>

/* common address space regions */
#define FPGASLAVES_BASE   (0xc0000000)
#define PERIPH_BASE       (0xfc000000)
#define LWPFGASLAVES_BASE (0xff200000)

/* hardware base addresses */
#define STM_BASE            (0xfc000000)
#define DAP_BASE            (0xff000000)
#define EMAC0_BASE          (0xff700000)
#define EMAC1_BASE          (0xff702000)
#define SDMMC_BASE          (0xff704000)
#define QSPI_BASE           (0xff705000)
#define FPGAMGRREGS_BASE    (0xff706000)
#define ACPIDMAP_BASE       (0xff707000)
#define GPIO0_BASE          (0xff708000)
#define GPIO1_BASE          (0xff709000)
#define GPIO2_BASE          (0xff70a000)
#define L3REGS_BASE         (0xff800000)
#define NANDDATA_BASE       (0xff900000)
#define QSPIDATA_BASE       (0xffa00000)
#define USB0_BASE           (0xffb00000)
#define USB1_BASE           (0xffb40000)
#define NANDREGS_BASE       (0xffb80000)
#define FPGAMGRDATA_BASE    (0xffb90000)
#define CAN0_BASE           (0xffc00000)
#define CAN1_BASE           (0xffc01000)
#define UART0_BASE          (0xffc02000)
#define UART1_BASE          (0xffc03000)
#define I2C0_BASE           (0xffc04000)
#define I2C1_BASE           (0xffc05000)
#define I2C2_BASE           (0xffc06000)
#define I2C3_BASE           (0xffc07000)
#define SPTIMER0_BASE       (0xffc08000)
#define SPTIMER1_BASE       (0xffc09000)
#define SDRREGS_BASE        (0xffc20000)
#define OSC1TIMER0_BASE     (0xffd00000)
#define OSC1TIMER1_BASE     (0xffd01000)
#define L4WD0_BASE          (0xffd02000)
#define L4WD1_BASE          (0xffd03000)
#define CLKMGR_BASE         (0xffd04000)
#define RSTMGR_BASE         (0xffd05000)
#define SYSMGR_BASE         (0xffd08000)
#define DMANONSECURE_BASE   (0xffe00000)
#define DMASECURE_BASE      (0xffe01000)
#define SPIS0_BASE          (0xffe02000)
#define SPIS1_BASE          (0xffe03000)
#define SPIM0_BASE          (0xfff00000)
#define SPIM1_BASE          (0xfff01000)
#define SCANMGR_BASE        (0xfff02000)
#define ROM_BASE            (0xfffd0000)
#define MPUSCU_BASE         (0xfffec000)
#define MPUL2_BASE          (0xfffef000)
#define OCRAM_BASE          (0xffff0000)

#define CPUPRIV_BASE        (MPUSCU_BASE)
#define SCU_CONTROL_BASE    (CPUPRIV_BASE + 0x0000)
#define GIC_PROC_BASE       (CPUPRIV_BASE + 0x0100)
#define GLOBAL_TIMER_BASE   (CPUPRIV_BASE + 0x0200)
#define PRIV_TIMER_BASE     (CPUPRIV_BASE + 0x0600)
#define GIC_DISTRIB_BASE    (CPUPRIV_BASE + 0x1000)

/* interrupts */

#define FPGA_INT(n) (72 + (n))

#define SPI0_INT        186
#define SPI1_INT        187
#define SPI2_INT        188
#define SPI3_INT        189
#define I2C0_INT        190
#define I2C1_INT        191
#define I2C2_INT        192
#define I2C3_INT        193
#define UART0_INT       194
#define UART1_INT       195
#define GPIO0_INT       196
#define GPIO1_INT       197
#define GPIO2_INT       198
#define TIMER_L4SP0_INT 199
#define TIMER_L4SP1_INT 200
#define TIMER_OSC0_INT  201
#define TIMER_OSC1_INT  202
#define WDOG0_INT       203
#define WDOG1_INT       204
#define CLKMGR_INT      205
#define MPUWAKEUP_INT   206
#define FPGA_MAN_INT    207

#define MAX_INT 212

