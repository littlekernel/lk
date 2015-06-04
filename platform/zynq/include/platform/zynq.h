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

#define ZYNQ_MIO_CNT    54

/* memory addresses */
/* assumes sram is mapped at 0 the first MB of sdram is covered by it */
#define SDRAM_BASE          (0x00100000)
#define SDRAM_APERTURE_SIZE (0x3ff00000)
#define SRAM_BASE           (0x0)
#define SRAM_BASE_HIGH      (0xfffc0000)
#define SRAM_APERTURE_SIZE  (0x00040000)
#define SRAM_SIZE           (0x00040000)

/* hardware base addresses */
#define UART0_BASE (0xe0000000)
#define UART1_BASE (0xe0001000)
#define USB0_BASE  (0xe0002000)
#define USB1_BASE  (0xe0003000)
#define I2C0_BASE  (0xe0004000)
#define I2C1_BASE  (0xe0005000)
#define SPI0_BASE  (0xe0006000)
#define SPI1_BASE  (0xe0007000)
#define CAN0_BASE  (0xe0008000)
#define CAN1_BASE  (0xe0009000)
#define GPIO_BASE  (0xe000a000)
#define GEM0_BASE  (0xe000b000) // gigabit eth controller
#define GEM1_BASE  (0xe000c000) // ""
#define QSPI_BASE  (0xe000d000)
#define SMCC_BASE  (0xe000e000) // PL353 shared memory controller

#define SD0_BASE   (0xe0100000)
#define SD1_BASE   (0xe0101000)

#define SLCR_BASE  (0xf8000000)
#define TTC0_BASE  (0xf8001000)
#define TTC1_BASE  (0xf8002000)
#define DMAC0_NS_BASE (0xf8004000)
#define DMAC0_S_BASE (0xf8003000)
#define SWDT_BASE  (0xf8005000)

#define CPUPRIV_BASE      (0xf8f00000)
#define SCU_CONTROL_BASE  (CPUPRIV_BASE + 0x0000)
#define GIC_PROC_BASE     (CPUPRIV_BASE + 0x0100)
#define GLOBAL_TIMER_BASE (CPUPRIV_BASE + 0x0200)
#define PRIV_TIMER_BASE   (CPUPRIV_BASE + 0x0600)
#define GIC_DISTRIB_BASE  (CPUPRIV_BASE + 0x1000)
#define L2CACHE_BASE      (CPUPRIV_BASE + 0x2000)

#define QSPI_LINEAR_BASE  (0xfc000000)

/* interrupts */
#define TTC0_A_INT    42
#define TTC0_B_INT    43
#define TTC0_C_INT    44
#define GPIO_INT      52
#define USB0_INT      53
#define ETH0_INT      54
#define ETH0_WAKE_INT 55
#define SDIO0_INT     56
#define I2C0_INT      57
#define SPI0_INT      58
#define UART0_INT     59
#define UART1_INT     82
#define TTC1_A_INT    69
#define TTC2_B_INT    70
#define TTC3_C_INT    71
#define ETH1_INT      77
#define ETH1_WAKE_INT 78

/* Perhipheral IRQs from fabric */
#define F2P0_IRQ      61
#define F2P1_IRQ      62
#define F2P2_IRQ      63
#define F2P3_IRQ      64
#define F2P4_IRQ      65
#define F2P5_IRQ      66
#define F2P6_IRQ      67
#define F2P7_IRQ      68

#define F2P8_IRQ      84
#define F2P9_IRQ      85
#define F2P10_IRQ     86
#define F2P11_IRQ     87
#define F2P12_IRQ     88
#define F2P13_IRQ     89
#define F2P14_IRQ     90
#define F2P15_IRQ     91

#define MAX_INT       96

#ifndef ASSEMBLY

#include <reg.h>
#include <compiler.h>
#include <bits.h>
#include <stdbool.h>
#include <sys/types.h>

/* Configuration values for each of the system PLLs. Refer to the TRM 25.10.4 */
typedef struct {
    uint32_t lock_cnt;
    uint32_t cp;
    uint32_t res;
    uint32_t fdiv;
} zynq_pll_cfg_t;

typedef struct {
    uint32_t arm_clk;
    uint32_t ddr_clk;
    uint32_t dci_clk;
    uint32_t gem0_clk;
    uint32_t gem0_rclk;
    uint32_t gem1_clk;
    uint32_t gem1_rclk;
    uint32_t smc_clk;
    uint32_t lqspi_clk;
    uint32_t sdio_clk;
    uint32_t uart_clk;
    uint32_t spi_clk;
    uint32_t can_clk;
    uint32_t can_mioclk;
    uint32_t usb0_clk;
    uint32_t usb1_clk;
    uint32_t pcap_clk;
    uint32_t fpga0_clk;
    uint32_t fpga1_clk;
    uint32_t fpga2_clk;
    uint32_t fpga3_clk;
    uint32_t aper_clk;
    uint32_t clk_621_true;
} zynq_clk_cfg_t;

typedef struct {
    zynq_pll_cfg_t arm;
    zynq_pll_cfg_t ddr;
    zynq_pll_cfg_t io;
} zynq_pll_cfg_tree_t;

/* Configuration for the DDR controller and buffers. TRM Ch 10 */
typedef struct {
    uint32_t addr0;
    uint32_t addr1;
    uint32_t data0;
    uint32_t data1;
    uint32_t diff0;
    uint32_t diff1;
    bool ibuf_disable;
    bool term_disable;
} zynq_ddriob_cfg_t;;

/* SLCR registers */
struct slcr_regs {
    uint32_t SCL;                             // Secure Configuration Lock
    uint32_t SLCR_LOCK;                       // SLCR Write Protection Lock
    uint32_t SLCR_UNLOCK;                     // SLCR Write Protection Unlock
    uint32_t SLCR_LOCKSTA;                    // SLCR Write Protection Status
    uint32_t ___reserved0[60];
    uint32_t ARM_PLL_CTRL;                    // ARM PLL Control
    uint32_t DDR_PLL_CTRL;                    // DDR PLL Control
    uint32_t IO_PLL_CTRL;                     // IO PLL Control
    uint32_t PLL_STATUS;                      // PLL Status
    uint32_t ARM_PLL_CFG;                     // ARM PLL Configuration
    uint32_t DDR_PLL_CFG;                     // DDR PLL Configuration
    uint32_t IO_PLL_CFG;                      // IO PLL Configuration
    uint32_t ___reserved1[1];
    uint32_t ARM_CLK_CTRL;                    // CPU Clock Control
    uint32_t DDR_CLK_CTRL;                    // DDR Clock Control
    uint32_t DCI_CLK_CTRL;                    // DCI clock control
    uint32_t APER_CLK_CTRL;                   // AMBA Peripheral Clock Control
    uint32_t USB0_CLK_CTRL;                   // USB 0 ULPI Clock Control
    uint32_t USB1_CLK_CTRL;                   // USB 1 ULPI Clock Control
    uint32_t GEM0_RCLK_CTRL;                  // GigE 0 Rx Clock and Rx Signals Select
    uint32_t GEM1_RCLK_CTRL;                  // GigE 1 Rx Clock and Rx Signals Select
    uint32_t GEM0_CLK_CTRL;                   // GigE 0 Ref Clock Control
    uint32_t GEM1_CLK_CTRL;                   // GigE 1 Ref Clock Control
    uint32_t SMC_CLK_CTRL;                    // SMC Ref Clock Control
    uint32_t LQSPI_CLK_CTRL;                  // Quad SPI Ref Clock Control
    uint32_t SDIO_CLK_CTRL;                   // SDIO Ref Clock Control
    uint32_t UART_CLK_CTRL;                   // UART Ref Clock Control
    uint32_t SPI_CLK_CTRL;                    // SPI Ref Clock Control
    uint32_t CAN_CLK_CTRL;                    // CAN Ref Clock Control
    uint32_t CAN_MIOCLK_CTRL;                 // CAN MIO Clock Control
    uint32_t DBG_CLK_CTRL;                    // SoC Debug Clock Control
    uint32_t PCAP_CLK_CTRL;                   // PCAP Clock Control
    uint32_t TOPSW_CLK_CTRL;                  // Central Interconnect Clock Control
    uint32_t FPGA0_CLK_CTRL;                  // PL Clock 0 Output control
    uint32_t FPGA0_THR_CTRL;                  // PL Clock 0 Throttle control
    uint32_t FPGA0_THR_CNT;                   // PL Clock 0 Throttle Count control
    uint32_t FPGA0_THR_STA;                   // PL Clock 0 Throttle Status read
    uint32_t FPGA1_CLK_CTRL;                  // PL Clock 1 Output control
    uint32_t FPGA1_THR_CTRL;                  // PL Clock 1 Throttle control
    uint32_t FPGA1_THR_CNT;                   // PL Clock 1 Throttle Count
    uint32_t FPGA1_THR_STA;                   // PL Clock 1 Throttle Status control
    uint32_t FPGA2_CLK_CTRL;                  // PL Clock 2 output control
    uint32_t FPGA2_THR_CTRL;                  // PL Clock 2 Throttle Control
    uint32_t FPGA2_THR_CNT;                   // PL Clock 2 Throttle Count
    uint32_t FPGA2_THR_STA;                   // PL Clock 2 Throttle Status
    uint32_t FPGA3_CLK_CTRL;                  // PL Clock 3 output control
    uint32_t FPGA3_THR_CTRL;                  // PL Clock 3 Throttle Control
    uint32_t FPGA3_THR_CNT;                   // PL Clock 3 Throttle Count
    uint32_t FPGA3_THR_STA;                   // PL Clock 3 Throttle Status
    uint32_t ___reserved2[5];
    uint32_t CLK_621_TRUE;                    // CPU Clock Ratio Mode select
    uint32_t ___reserved3[14];
    uint32_t PSS_RST_CTRL;                    // PS Software Reset Control
    uint32_t DDR_RST_CTRL;                    // DDR Software Reset Control
    uint32_t TOPSW_RST_CTRL;                  // Central Interconnect Reset Control
    uint32_t DMAC_RST_CTRL;                   // DMAC Software Reset Control
    uint32_t USB_RST_CTRL;                    // USB Software Reset Control
    uint32_t GEM_RST_CTRL;                    // Gigabit Ethernet SW Reset Control
    uint32_t SDIO_RST_CTRL;                   // SDIO Software Reset Control
    uint32_t SPI_RST_CTRL;                    // SPI Software Reset Control
    uint32_t CAN_RST_CTRL;                    // CAN Software Reset Control
    uint32_t I2C_RST_CTRL;                    // I2C Software Reset Control
    uint32_t UART_RST_CTRL;                   // UART Software Reset Control
    uint32_t GPIO_RST_CTRL;                   // GPIO Software Reset Control
    uint32_t LQSPI_RST_CTRL;                  // Quad SPI Software Reset Control
    uint32_t SMC_RST_CTRL;                    // SMC Software Reset Control
    uint32_t OCM_RST_CTRL;                    // OCM Software Reset Control
    uint32_t ___reserved4[1];
    uint32_t FPGA_RST_CTRL;                   // FPGA Software Reset Control
    uint32_t A9_CPU_RST_CTRL;                 // CPU Reset and Clock control
    uint32_t ___reserved5[1];
    uint32_t RS_AWDT_CTRL;                    // Watchdog Timer Reset Control
    uint32_t ___reserved6[2];
    uint32_t REBOOT_STATUS;                   // Reboot Status, persistent
    uint32_t BOOT_MODE;                       // Boot Mode Strapping Pins
    uint32_t ___reserved7[40];
    uint32_t APU_CTRL;                        // APU Control
    uint32_t WDT_CLK_SEL;                     // SWDT clock source select
    uint32_t ___reserved8[78];
    uint32_t TZ_DMA_NS;                       // DMAC TrustZone Config
    uint32_t TZ_DMA_IRQ_NS;                   // DMAC TrustZone Config for Interrupts
    uint32_t TZ_DMA_PERIPH_NS;                // DMAC TrustZone Config for Peripherals
    uint32_t ___reserved9[57];
    uint32_t PSS_IDCODE;                      // PS IDCODE
    uint32_t ___reserved10[51];
    uint32_t DDR_URGENT;                      // DDR Urgent Control
    uint32_t ___reserved11[2];
    uint32_t DDR_CAL_START;                   // DDR Calibration Start Triggers
    uint32_t ___reserved12[1];
    uint32_t DDR_REF_START;                   // DDR Refresh Start Triggers
    uint32_t DDR_CMD_STA;                     // DDR Command Store Status
    uint32_t DDR_URGENT_SEL;                  // DDR Urgent Select
    uint32_t DDR_DFI_STATUS;                  // DDR DFI status
    uint32_t ___reserved13[55];
    uint32_t MIO_PIN_00;                      // MIO Pin 0 Control
    uint32_t MIO_PIN_01;                      // MIO Pin 1 Control
    uint32_t MIO_PIN_02;                      // MIO Pin 2 Control
    uint32_t MIO_PIN_03;                      // MIO Pin 3 Control
    uint32_t MIO_PIN_04;                      // MIO Pin 4 Control
    uint32_t MIO_PIN_05;                      // MIO Pin 5 Control
    uint32_t MIO_PIN_06;                      // MIO Pin 6 Control
    uint32_t MIO_PIN_07;                      // MIO Pin 7 Control
    uint32_t MIO_PIN_08;                      // MIO Pin 8 Control
    uint32_t MIO_PIN_09;                      // MIO Pin 9 Control
    uint32_t MIO_PIN_10;                      // MIO Pin 10 Control
    uint32_t MIO_PIN_11;                      // MIO Pin 11 Control
    uint32_t MIO_PIN_12;                      // MIO Pin 12 Control
    uint32_t MIO_PIN_13;                      // MIO Pin 13 Control
    uint32_t MIO_PIN_14;                      // MIO Pin 14 Control
    uint32_t MIO_PIN_15;                      // MIO Pin 15 Control
    uint32_t MIO_PIN_16;                      // MIO Pin 16 Control
    uint32_t MIO_PIN_17;                      // MIO Pin 17 Control
    uint32_t MIO_PIN_18;                      // MIO Pin 18 Control
    uint32_t MIO_PIN_19;                      // MIO Pin 19 Control
    uint32_t MIO_PIN_20;                      // MIO Pin 20 Control
    uint32_t MIO_PIN_21;                      // MIO Pin 21 Control
    uint32_t MIO_PIN_22;                      // MIO Pin 22 Control
    uint32_t MIO_PIN_23;                      // MIO Pin 23 Control
    uint32_t MIO_PIN_24;                      // MIO Pin 24 Control
    uint32_t MIO_PIN_25;                      // MIO Pin 25 Control
    uint32_t MIO_PIN_26;                      // MIO Pin 26 Control
    uint32_t MIO_PIN_27;                      // MIO Pin 27 Control
    uint32_t MIO_PIN_28;                      // MIO Pin 28 Control
    uint32_t MIO_PIN_29;                      // MIO Pin 29 Control
    uint32_t MIO_PIN_30;                      // MIO Pin 30 Control
    uint32_t MIO_PIN_31;                      // MIO Pin 31 Control
    uint32_t MIO_PIN_32;                      // MIO Pin 32 Control
    uint32_t MIO_PIN_33;                      // MIO Pin 33 Control
    uint32_t MIO_PIN_34;                      // MIO Pin 34 Control
    uint32_t MIO_PIN_35;                      // MIO Pin 35 Control
    uint32_t MIO_PIN_36;                      // MIO Pin 36 Control
    uint32_t MIO_PIN_37;                      // MIO Pin 37 Control
    uint32_t MIO_PIN_38;                      // MIO Pin 38 Control
    uint32_t MIO_PIN_39;                      // MIO Pin 39 Control
    uint32_t MIO_PIN_40;                      // MIO Pin 40 Control
    uint32_t MIO_PIN_41;                      // MIO Pin 41 Control
    uint32_t MIO_PIN_42;                      // MIO Pin 42 Control
    uint32_t MIO_PIN_43;                      // MIO Pin 43 Control
    uint32_t MIO_PIN_44;                      // MIO Pin 44 Control
    uint32_t MIO_PIN_45;                      // MIO Pin 45 Control
    uint32_t MIO_PIN_46;                      // MIO Pin 46 Control
    uint32_t MIO_PIN_47;                      // MIO Pin 47 Control
    uint32_t MIO_PIN_48;                      // MIO Pin 48 Control
    uint32_t MIO_PIN_49;                      // MIO Pin 49 Control
    uint32_t MIO_PIN_50;                      // MIO Pin 50 Control
    uint32_t MIO_PIN_51;                      // MIO Pin 51 Control
    uint32_t MIO_PIN_52;                      // MIO Pin 52 Control
    uint32_t MIO_PIN_53;                      // MIO Pin 53 Control
    uint32_t ___reserved14[11];
    uint32_t MIO_LOOPBACK;                    // Loopback function within MIO
    uint32_t ___reserved15[1];
    uint32_t MIO_MST_TRI0;                    // MIO pin Tri-state Enables, 31:0
    uint32_t MIO_MST_TRI1;                    // MIO pin Tri-state Enables, 53:32
    uint32_t ___reserved16[7];
    uint32_t SD0_WP_CD_SEL;                   // SDIO 0 WP CD select
    uint32_t SD1_WP_CD_SEL;                   // SDIO 1 WP CD select
    uint32_t ___reserved17[50];
    uint32_t LVL_SHFTR_EN;                    // Level Shifters Enable
    uint32_t ___reserved18[3];
    uint32_t OCM_CFG;                         // OCM Address Mapping
    uint32_t ___reserved19[66];
    uint32_t RESERVED;                        // Reserved
    uint32_t ___reserved20[56];
    uint32_t GPIOB_CTRL;                      // PS IO Buffer Control
    uint32_t GPIOB_CFG_CMOS18;                // MIO GPIOB CMOS 1.8V config
    uint32_t GPIOB_CFG_CMOS25;                // MIO GPIOB CMOS 2.5V config
    uint32_t GPIOB_CFG_CMOS33;                // MIO GPIOB CMOS 3.3V config
    uint32_t ___reserved21[1];
    uint32_t GPIOB_CFG_HSTL;                  // MIO GPIOB HSTL config
    uint32_t GPIOB_DRVR_BIAS_CTRL;            // MIO GPIOB Driver Bias Control
    uint32_t ___reserved22[9];
    uint32_t DDRIOB_ADDR0;                    // DDR IOB Config for A[14:0], CKE and DRST_B
    uint32_t DDRIOB_ADDR1;                    // DDR IOB Config for BA[2:0], ODT, CS_B, WE_B, RAS_B and CAS_B
    uint32_t DDRIOB_DATA0;                    // DDR IOB Config for Data 15:0
    uint32_t DDRIOB_DATA1;                    // DDR IOB Config for Data 31:16
    uint32_t DDRIOB_DIFF0;                    // DDR IOB Config for DQS 1:0
    uint32_t DDRIOB_DIFF1;                    // DDR IOB Config for DQS 3:2
    uint32_t DDRIOB_CLOCK;                    // DDR IOB Config for Clock Output
    uint32_t DDRIOB_DRIVE_SLEW_ADDR;          // Drive and Slew controls for Address and Command pins of the DDR Interface
    uint32_t DDRIOB_DRIVE_SLEW_DATA;          // Drive and Slew controls for DQ pins of the DDR Interface
    uint32_t DDRIOB_DRIVE_SLEW_DIFF;          // Drive and Slew controls for DQS pins of the DDR Interface
    uint32_t DDRIOB_DRIVE_SLEW_CLOCK;         // Drive and Slew controls for Clock pins of the DDR Interface
    uint32_t DDRIOB_DDR_CTRL;                 // DDR IOB Buffer Control
    uint32_t DDRIOB_DCI_CTRL;                 // DDR IOB DCI Config
    uint32_t DDRIOB_DCI_STATUS;               // DDR IO Buffer DCI Status
};

/* Verify the entries match the TRM offset to validate the struct */
STATIC_ASSERT(offsetof(struct slcr_regs, SCL) == 0x0);
STATIC_ASSERT(offsetof(struct slcr_regs, DDRIOB_DCI_STATUS) == 0xb74);

#define DDRC_CTRL                       0xF8006000
#define DDRC_MODE_STATUS                0xF8006054

#define DDRC_CTRL_OUT_OF_RESET          (1)
#define DDRC_CTRL_BUS_WIDTH_16BIT       (1 << 2)
#define DDRC_CTRL_RDRW_IDLE_GAP(x)      ((x & BIT_MASK(7) << 7)

#define DDRC_STS_OPER_MODE(x)           (x & BIT_MASK(3))
#define DDRC_STS_SELF_REFRESH           DDRC_STS_OPER_MODE(0x3)

#define SLCR                            ((volatile struct slcr_regs *)SLCR_BASE)
#define SLCR_REG(reg)                   (*REG32((uintptr_t)&SLCR->reg))

/* ARM_PLL_CFG */
#define PLL_CFG_PLL_RES(x)              ((x & BIT_MASK(4)) << 4)
#define PLL_CFG_PLL_CP(x)               ((x & BIT_MASK(4)) << 8)
#define PLL_CFG_LOCK_CNT(x)             ((x & BIT_MASK(10)) << 12)

/* DDR_PLL_CFG */

/* ARM_PLL_CTRL and IO_PLL_CTRL */
#define PLL_RESET                       (1)
#define PLL_PWRDOWN                     (1 << 1)
#define PLL_BYPASS_QUAL                 (1 << 3)
#define PLL_BYPASS_FORCE                (1 << 4)
#define PLL_FDIV(x)                     ((x & BIT_MASK(7)) << 12)

/* ARM_CLK_CTRL */
#define ARM_CLK_CTRL_SRCSEL(x)          ((x & BIT_MASK(2)) << 4)
#define ARM_CLK_CTRL_DIVISOR(x)         ((x & BIT_MASK(6)) << 8)
#define ARM_CLK_CTRL_CPU_6OR4XCLKACT    (1 << 24)
#define ARM_CLK_CTRL_CPU_3OR2XCLKACT    (1 << 25)
#define ARM_CLK_CTRL_CPU_2XCLKACT       (1 << 26)
#define ARM_CLK_CTRL_CPU_1XCLKACT       (1 << 27)
#define ARM_CLK_CTRL_PERI_CLKACT        (1 << 28)

/* DDR_CLK_CTRL */
#define DDR_CLK_CTRL_DDR_3XCLKACT       (1)
#define DDR_CLK_CTRL_DDR_2XCLKACT       (1 << 1)
#define DDR_CLK_CTRL_DDR_3XCLK_DIV(x)   ((x & BIT_MASK(6)) << 20)
#define DDR_CLK_CTRL_DDR_2XCLK_DIV(x)   ((x & BIT_MASK(6)) << 26)

/* PLL_STATUS */
#define PLL_STATUS_ARM_PLL_LOCK         (1)
#define PLL_STATUS_DDR_PLL_LOCK         (1 << 1)
#define PLL_STATUS_IO_PLL_LOCK          (1 << 2)
#define PLL_STATUS_ARM_PLL_STABLE       (1 << 3)
#define PLL_STATUS_DDR_PLL_STABLE       (1 << 4)
#define PLL_STATUS_IO_PLL_STABLE        (1 << 5)

/* Generic clock control */
#define CLK_CTRL_CLKACT                 (1)
#define CLK_CTRL_CLKACT1                (1 << 1)
#define CLK_CTRL_SRCSEL(x)              ((x & BIT_MASK(2)) << 4)
#define CLK_CTRL_DIVISOR0(x)            ((x & BIT_MASK(6)) << 8)
#define CLK_CTRL_DIVISOR1(x)            ((x & BIT_MASK(6)) << 20)

/* GEM clock control */
#define GEM_CLK_CTRL_SRCSEL(x)          ((x & BIT_MASK(3)) << 4)

/* CLK 621 just has a single enable bit */
#define CLK_621_ENABLE                  (1)

/* AMBA Peripheral Clock Control */
#define SMC_CPU_CLK_EN                  (1 << 24)
#define LQSPI_CPU_CLK_EN                (1 << 23)
#define GPIO_CPU_CLK_EN                 (1 << 22)
#define UART1_CPU_CLK_EN                (1 << 21)
#define UART0_CPU_CLK_EN                (1 << 20)
#define I2C1_CPU_CLK_EN                 (1 << 19)
#define I2C0_CPU_CLK_EN                 (1 << 18)
#define CAN1_CPU_CLK_EN                 (1 << 17)
#define CAN0_CPU_CLK_EN                 (1 << 16)
#define SPI1_CPU_CLK_EN                 (1 << 15)
#define SPI0_CPU_CLK_EN                 (1 << 14)
#define SDI1_CPU_CLK_EN                 (1 << 11)
#define SDI0_CPU_CLK_EN                 (1 << 10)
#define GEM1_CPU_CLK_EN                 (1 << 7)
#define GEM0_CPU_CLK_EN                 (1 << 6)
#define USB1_CPU_CLK_EN                 (1 << 3)
#define USB0_CPU_CLK_EN                 (1 << 2)
#define DMA_CPU_CLK_EN                  (1 << 0)

/* GPIOB_CTRL */
#define GPIOB_CTRL_VREF_09_EN           (1 << 4)
#define GPIOB_CTRL_VREF_EN              (1)

/* DDRIOB_ADDR */
#define DDRIOB_PULLUP_EN                (1 << 11)
#define DDRIOB_OUTPUT_EN(x)             ((x & BIT_MASK(2)) << 9)
#define DDRIOB_TERM_DISABLE_MODE        (1 << 8)
#define DDRIOB_IBUF_DISABLE_MODE        (1 << 7)
#define DDRIOB_DCI_TYPE(x)              ((x & BIT_MASK(2)) << 5)
#define DDRIOB_TERM_EN                  (1 << 4)
#define DDRIOB_DCI_UPDATE_B             (1 << 3)
#define DDRIOB_INP_TYPE(x)              ((x & BIT_MASK(2)) << 1)

/* SD1_WP_CD_SEL */
#define SDIO0_WP_SEL(x)                 (x & BIT_MASK(6))
#define SDIO0_CD_SEL(x)                 ((x & BIT_MASK(6)) << 16)

/* MIO pin configuration */
#define MIO_TRI_ENABLE                  (1)
#define MIO_L0_SEL                      (1 << 1)
#define MIO_L1_SEL                      (1 << 2)
#define MIO_L2_SEL(x)                   ((x & BIT_MASK(2)) << 3)
#define MIO_L2_SEL_MASK                 MIO_L2_SEL(0x3)
#define MIO_L3_SEL(x)                   ((x & BIT_MASK(3)) << 5)
#define MIO_L3_SEL_MASK                 MIO_L3_SEL(0x7)
#define MIO_SPEED_FAST                  (1 << 8)
#define MIO_IO_TYPE_LVCMOS18            (0x1 << 9)
#define MIO_IO_TYPE_LVCMOS25            (0x2 << 9)
#define MIO_IO_TYPE_LVCMOS33            (0x3 << 9)
#define MIO_IO_TYPE_HSTL                (0x4 << 9)
#define MIO_IO_TYPE_MASK                (0x7 << 9)
#define MIO_PULLUP                      (1 << 12)
#define MIO_DISABLE_RCVR                (1 << 13)
#define MIO_DEFAULT                     (0xFFFF0000)

/* UART registers */
#define UART_CR                         (0x00)
#define UART_MR                         (0x04)
#define UART_IER                        (0x08)
#define UART_IDR                        (0x0c)
#define UART_IMR                        (0x10)
#define UART_ISR                        (0x14)
#define UART_BAUDGEN                    (0x18)
#define UART_RXTOUT                     (0x1c)
#define UART_RXWM                       (0x20)
#define UART_MODEMCR                    (0x24)
#define UART_MODEMSR                    (0x28)
#define UART_SR                         (0x2c)
#define UART_FIFO                       (0x30)
#define UART_BAUD_DIV                   (0x34)
#define UART_FLOW_DELAY                 (0x38)
#define UART_TX_FIFO_TRIGGER            (0x44)

#define NUM_UARTS 2

#define UART_CR_RXRES                   (1)
#define UART_CR_TXRES                   (1 << 1)
#define UART_CR_RXEN                    (1 << 2)
#define UART_CR_RXDIS                   (1 << 3)
#define UART_CR_TXEN                    (1 << 4)
#define UART_CR_TXDIS                   (1 << 5)
#define UART_CR_RSTTO                   (1 << 6)
#define UART_CR_STTBRK                  (1 << 7)
#define UART_CR_STPBRK                  (1 << 8)

#define UART_MR_CLKS_DIV8               (1)
#define UART_MR_CHRL(x)                 ((x & BIT_MASK(2)) << 1)
#define UART_MR_PAR(x)                  ((x & BIT_MASK(3)) << 3)
#define UART_MR_NBSTOP(x)               ((x & BIT_MASK(2)) << 6)
#define UART_MR_CHMODE(x)               ((x & BIT_MASK(2)) << 8)

#define UART_BRG_DIV(x)                 (x & BIT_MASK(16))
#define UART_BRD_DIV(x)                 (x & BIT_MASK(8))

/* system watchdog timer */
struct swdt_regs {
    uint32_t MODE;
    uint32_t CONTROL;
    uint32_t RESTART;
    uint32_t STATUS;
};

#define SWDT                            ((volatile struct swdt_regs *)SWDT_BASE)
#define SWDT_REG(reg)                   (*REG32((uintptr_t)&SWDT->reg))

/* zynq specific functions */
static inline void zynq_slcr_unlock(void) { SLCR->SLCR_UNLOCK = 0xdf0d; }
static inline void zynq_slcr_lock(void) { SLCR->SLCR_LOCK = 0x767b; }

uint32_t zynq_get_arm_freq(void);
uint32_t zynq_get_arm_timer_freq(void);
uint32_t zynq_get_swdt_freq(void);
void zynq_dump_clocks(void);

enum zynq_clock_source {
    PLL_IO = 0,
    PLL_CPU = 2,
    PLL_DDR = 3,
};

enum zynq_periph {
    PERIPH_USB0,
    PERIPH_USB1,
    PERIPH_GEM0,
    PERIPH_GEM1,
    PERIPH_SMC,
    PERIPH_LQSPI,
    PERIPH_SDIO0,
    PERIPH_SDIO1,
    PERIPH_UART0,
    PERIPH_UART1,
    PERIPH_SPI0,
    PERIPH_SPI1,
    PERIPH_CAN0,
    PERIPH_CAN1,
    PERIPH_DBG,
    PERIPH_PCAP,
    PERIPH_FPGA0,
    PERIPH_FPGA1,
    PERIPH_FPGA2,
    PERIPH_FPGA3,

    _PERIPH_MAX,
};

status_t zynq_set_clock(enum zynq_periph, bool enable, enum zynq_clock_source, uint32_t divisor, uint32_t divisor2);
uint32_t zynq_get_clock(enum zynq_periph);

/* boot mode */
#define ZYNQ_BOOT_MODE_JTAG     (0)
#define ZYNQ_BOOT_MODE_QSPI     (1)
#define ZYNQ_BOOT_MODE_NOR      (2)
#define ZYNQ_BOOT_MODE_NAND     (4)
#define ZYNQ_BOOT_MODE_SD       (5)
#define ZYNQ_BOOT_MODE_MASK     (0x7)    /* only interested in BOOT_MODE[2:0] */

static inline uint32_t zynq_get_boot_mode(void) { return SLCR->BOOT_MODE & ZYNQ_BOOT_MODE_MASK; }

#endif // !ASSEMBLY

