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
#include <stdbool.h>
#include <sys/types.h>

/* memory addresses */
#define SDRAM_BASE          (0)
#define SDRAM_APERTURE_SIZE (0x40000000)
#define SRAM_BASE           (0xfffc0000)
#define SRAM_APERTURE_SIZE  (0x00040000)

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

/* interrupts */
#define TTC0_A_INT  42
#define TTC0_B_INT  43
#define TTC0_C_INT  44
#define UART0_INT   59
#define UART1_INT   82
#define TTC1_A_INT  69
#define TTC2_B_INT  70
#define TTC3_C_INT  71

#define MAX_INT 96

/* UART registers */
#define UART_CR (0x00)
#define UART_MR (0x04)
#define UART_IER (0x08)
#define UART_IDR (0x0c)
#define UART_IMR (0x10)
#define UART_ISR (0x14)
#define UART_BAUDGEN (0x18)
#define UART_RXTOUT (0x1c)
#define UART_RXWM (0x20)
#define UART_MODEMCR (0x24)
#define UART_MODEMSR (0x28)
#define UART_SR (0x2c)
#define UART_FIFO (0x30)
#define UART_BAUD_DIV (0x34)
#define UART_FLOW_DELAY (0x38)
#define UART_TX_FIFO_TRIGGER (0x44)

#define NUM_UARTS 2

/* SLCR registers */
#define SLCR_REG(reg) (*REG32(SLCR_BASE + (reg)))

#define SCL         0x00000000
#define SLCR_LOCK   0x00000004
#define SLCR_UNLOCK 0x00000008
#define SLCR_LOCKSTA 0x0000000c
#define ARM_PLL_CTRL 0x00000100
#define DDR_PLL_CTRL 0x00000104
#define IO_PLL_CTRL 0x00000108
#define PLL_STATUS 0x0000010c
#define ARM_PLL_CFG 0x00000110
#define DDR_PLL_CFG 0x00000114
#define IO_PLL_CFG 0x00000118
#define ARM_CLK_CTRL 0x00000120
#define DDR_CLK_CTRL 0x00000124
#define IO_CLK_CTRL 0x00000128
#define APER_CLK_CTRL 0x0000012c
#define USB0_CLK_CTRL 0x00000130
#define USB1_CLK_CTRL 0x00000134
#define GEM0_RCLK_CTRL 0x00000138
#define GEM1_RCLK_CTRL 0x0000013c
#define GEM0_CLK_CTRL 0x00000140
#define GEM1_CLK_CTRL 0x00000144
#define SMC_CLK_CTRL 0x00000148
#define LQSPI_CLK_CTRL 0x0000014c
#define SDIO_CLK_CTRL 0x00000150
#define UART_CLK_CTRL 0x00000154
#define SPI_CLK_CTRL 0x00000158
#define CAN_CLK_CTRL 0x0000015C
#define CAN_MIOCLK_CTRL 0x00000160
#define DBG_CLK_CTRL 0x00000164
#define PCAP_CLK_CTRL 0x00000168
#define TOPSW_CLK_CTRL 0x0000016C
#define FPGA0_CLK_CTRL 0x00000170
#define FPGA0_THR_CTRL 0x00000174
#define FPGA0_THR_CNT 0x00000178
#define FPGA0_THR_STA 0x0000017C
#define FPGA1_CLK_CTRL 0x00000180
#define FPGA1_THR_CTRL 0x00000184
#define FPGA1_THR_CNT 0x00000188
#define FPGA1_THR_STA 0x0000018C
#define FPGA2_CLK_CTRL 0x00000190
#define FPGA2_THR_CTRL 0x00000194
#define FPGA2_THR_CNT 0x00000198
#define FPGA2_THR_STA 0x0000019C
#define FPGA3_CLK_CTRL 0x000001A0
#define FPGA3_THR_CTRL 0x000001A4
#define FPGA3_THR_CNT 0x000001A8
#define FPGA3_THR_STA 0x000001AC
#define CLK_621_TRUE 0x000001C4
#define PSS_RST_CTRL 0x00000200
#define DDR_RST_CTRL 0x00000204
#define TOPSW_RST_CTRL 0x00000208
#define DMAC_RST_CTRL 0x0000020C
#define USB_RST_CTRL 0x00000210
#define GEM_RST_CTRL 0x00000214
#define SDIO_RST_CTRL 0x00000218
#define SPI_RST_CTRL 0x0000021C
#define CAN_RST_CTRL 0x00000220
#define I2C_RST_CTRL 0x00000224
#define UART_RST_CTRL 0x00000228
#define GPIO_RST_CTRL 0x0000022C
#define LQSPI_RST_CTRL 0x00000230
#define SMC_RST_CTRL 0x00000234
#define OCM_RST_CTRL 0x00000238
#define FPGA_RST_CTRL 0x00000240
#define A9_CPU_RST_CTRL 0x00000244
#define RS_AWDT_CTRL 0x0000024C
#define REBOOT_STATUS 0x00000258
#define BOOT_MODE 0x0000025C
#define APU_CTRL 0x00000300
#define WDT_CLK_SEL 0x00000304
#define TZ_DMA_NS 0x00000440
#define TZ_DMA_IRQ_NS 0x00000444
#define TZ_DMA_PERIPH_NS 0x00000448
#define PSS_IDCODE 0x00000530
#define DDR_URGENT 0x00000600
#define DDR_CAL_START 0x0000060C
#define DDR_REF_START 0x00000614
#define DDR_CMD_STA 0x00000618
#define DDR_URGENT_SEL 0x0000061C
#define DDR_DFI_STATUS 0x00000620
#define MIO_PIN_00 0x00000700
#define MIO_PIN_01 0x00000704
#define MIO_PIN_02 0x00000708
#define MIO_PIN_03 0x0000070C
#define MIO_PIN_04 0x00000710
#define MIO_PIN_05 0x00000714
#define MIO_PIN_06 0x00000718
#define MIO_PIN_07 0x0000071C
#define MIO_PIN_08 0x00000720
#define MIO_PIN_09 0x00000724
#define MIO_PIN_10 0x00000728
#define MIO_PIN_11 0x0000072C
#define MIO_PIN_12 0x00000730
#define MIO_PIN_13 0x00000734
#define MIO_PIN_14 0x00000738
#define MIO_PIN_15 0x0000073C
#define MIO_PIN_16 0x00000740
#define MIO_PIN_17 0x00000744
#define MIO_PIN_18 0x00000748
#define MIO_PIN_19 0x0000074C
#define MIO_PIN_20 0x00000750
#define MIO_PIN_21 0x00000754
#define MIO_PIN_22 0x00000758
#define MIO_PIN_23 0x0000075C
#define MIO_PIN_24 0x00000760
#define MIO_PIN_25 0x00000764
#define MIO_PIN_26 0x00000768
#define MIO_PIN_27 0x0000076C
#define MIO_PIN_28 0x00000770
#define MIO_PIN_29 0x00000774
#define MIO_PIN_30 0x00000778
#define MIO_PIN_31 0x0000077C
#define MIO_PIN_32 0x00000780
#define MIO_PIN_33 0x00000784
#define MIO_PIN_34 0x00000788
#define MIO_PIN_35 0x0000078C
#define MIO_PIN_36 0x00000790
#define MIO_PIN_37 0x00000794
#define MIO_PIN_38 0x00000798
#define MIO_PIN_39 0x0000079C
#define MIO_PIN_40 0x000007A0
#define MIO_PIN_41 0x000007A4
#define MIO_PIN_42 0x000007A8
#define MIO_PIN_43 0x000007AC
#define MIO_PIN_44 0x000007B0
#define MIO_PIN_45 0x000007B4
#define MIO_PIN_46 0x000007B8
#define MIO_PIN_47 0x000007BC
#define MIO_PIN_48 0x000007C0
#define MIO_PIN_49 0x000007C4
#define MIO_PIN_50 0x000007C8
#define MIO_PIN_51 0x000007CC
#define MIO_PIN_52 0x000007D0
#define MIO_PIN_53 0x000007D4
#define MIO_LOOPBACK 0x00000804
#define MIO_MST_TRI0 0x0000080C
#define MIO_MST_TRI1 0x00000810
#define SD0_WP_CD_SEL 0x00000830
#define SD1_WP_CD_SEL 0x00000834
#define LVL_SHFTR_EN 0x00000900
#define OCM_CFG 0x00000910
#define Reserved 0x00000A1C
#define GPIOB_CTRL 0x00000B00
#define GPIOB_CFG_CMOS18 0x00000B04
#define GPIOB_CFG_CMOS25 0x00000B08
#define GPIOB_CFG_CMOS33 0x00000B0C
#define GPIOB_CFG_HSTL 0x00000B14
#define GPIOB_DRVR_BIAS_C 0x00000B18
#define DDRIOB_ADDR0 0x00000B40
#define DDRIOB_ADDR1 0x00000B44
#define DDRIOB_DATA0 0x00000B48
#define DDRIOB_DATA1 0x00000B4C
#define DDRIOB_DIFF0 0x00000B50
#define DDRIOB_DIFF1 0x00000B54
#define DDRIOB_CLOCK 0x00000B58
#define DDRIOB_DRIVE_SLEW_ADDR 0x00000B5C
#define DDRIOB_DRIVE_SLEW_DATA 0x00000B60
#define DDRIOB_DRIVE_SLEW_DIFF 0x00000B64
#define DDRIOB_DRIVE_SLEW_CLOCK 0x00000B68
#define DDRIOB_DDR_CTRL 0x00000B6C
#define DDRIOB_DCI_CTRL 0x00000B70
#define DDRIOB_DCI_STATU 0x00000B74

static inline void zynq_slcr_unlock(void) { SLCR_REG(SLCR_UNLOCK) = 0xdf0d; }
static inline void zynq_slcr_lock(void) { SLCR_REG(SLCR_LOCK) = 0x767b; }

/* zynq specific functions */
uint32_t zynq_get_arm_freq(void);
uint32_t zynq_get_arm_timer_freq(void);
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

