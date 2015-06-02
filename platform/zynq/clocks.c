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
#include <reg.h>
#include <bits.h>
#include <stdio.h>
#include <assert.h>
#include <trace.h>
#include <err.h>
#include <kernel/thread.h>
#include <platform/debug.h>
#include <platform/zynq.h>
#include <target/debugconfig.h>
#include <reg.h>

#define LOCAL_TRACE 0

static uint32_t get_arm_pll_freq(void)
{
    LTRACEF("ARM_PLL_CTRL 0x%x\n", SLCR_REG(ARM_PLL_CTRL));

    // XXX test that the pll is actually enabled

    uint32_t fdiv = BITS_SHIFT(SLCR_REG(ARM_PLL_CTRL), 18, 12);

    return EXTERNAL_CLOCK_FREQ * fdiv;
}

static uint32_t get_ddr_pll_freq(void)
{
    LTRACEF("DDR_PLL_CTRL 0x%x\n", SLCR_REG(DDR_PLL_CTRL));

    // XXX test that the pll is actually enabled

    uint32_t fdiv = BITS_SHIFT(SLCR_REG(DDR_PLL_CTRL), 18, 12);

    return EXTERNAL_CLOCK_FREQ * fdiv;
}

static uint32_t get_io_pll_freq(void)
{
    LTRACEF("IO_PLL_CTRL 0x%x\n", SLCR_REG(IO_PLL_CTRL));

    // XXX test that the pll is actually enabled

    uint32_t fdiv = BITS_SHIFT(SLCR_REG(IO_PLL_CTRL), 18, 12);

    return EXTERNAL_CLOCK_FREQ * fdiv;
}

static uint32_t get_cpu_input_freq(void)
{
    LTRACEF("ARM_CLK_CTRL 0x%x\n", SLCR_REG(ARM_CLK_CTRL));

    uint32_t divisor = BITS_SHIFT(SLCR_REG(ARM_CLK_CTRL), 13, 8);
    uint32_t srcsel = BITS_SHIFT(SLCR_REG(ARM_CLK_CTRL), 5, 4);

    uint32_t srcclk;
    switch (srcsel) {
        default: case 0: case 1: // arm pll
            srcclk = get_arm_pll_freq();
            break;
        case 2: // ddr pll
            srcclk = get_ddr_pll_freq();
            break;
        case 3: // io pll
            srcclk = get_io_pll_freq();
            break;
    }

    // cpu 6x4x
    return srcclk / divisor;
}

static uint32_t get_cpu_6x4x_freq(void)
{
    // cpu 6x4x is the post divided frequency in the cpu clock block
    return get_cpu_input_freq();
}

static uint32_t get_cpu_3x2x_freq(void)
{
    // cpu 3x2x is always half the speed of 6x4x
    return get_cpu_input_freq() / 2;
}

static uint32_t get_cpu_2x_freq(void)
{
    // cpu 2x is either /3 or /2 the speed of 6x4x
    return get_cpu_input_freq() / ((SLCR_REG(CLK_621_TRUE) & 1) ? 3 : 2);
}

static uint32_t get_cpu_1x_freq(void)
{
    // cpu 1x is either /6 or /4 the speed of 6x4x
    return get_cpu_input_freq() / ((SLCR_REG(CLK_621_TRUE) & 1) ? 6 : 4);
}

uint32_t zynq_get_arm_freq(void)
{
    return get_cpu_6x4x_freq();
}

uint32_t zynq_get_arm_timer_freq(void)
{
    return get_cpu_3x2x_freq();
}

uint32_t zynq_get_swdt_freq(void)
{
    return get_cpu_1x_freq();
}

struct periph_clock {
    addr_t clk_ctrl_reg;
    uint enable_bit_pos;
};

static addr_t periph_clk_ctrl_reg(enum zynq_periph periph)
{
    DEBUG_ASSERT(periph < _PERIPH_MAX);

    switch (periph) {
        case PERIPH_USB0:   return (uintptr_t)&SLCR->USB0_CLK_CTRL;
        case PERIPH_USB1:   return (uintptr_t)&SLCR->USB1_CLK_CTRL;
        case PERIPH_GEM0:   return (uintptr_t)&SLCR->GEM0_CLK_CTRL;
        case PERIPH_GEM1:   return (uintptr_t)&SLCR->GEM1_CLK_CTRL;
        case PERIPH_SMC:    return (uintptr_t)&SLCR->SMC_CLK_CTRL;
        case PERIPH_LQSPI:  return (uintptr_t)&SLCR->LQSPI_CLK_CTRL;
        case PERIPH_SDIO0:  return (uintptr_t)&SLCR->SDIO_CLK_CTRL;
        case PERIPH_SDIO1:  return (uintptr_t)&SLCR->SDIO_CLK_CTRL;
        case PERIPH_UART0:  return (uintptr_t)&SLCR->UART_CLK_CTRL;
        case PERIPH_UART1:  return (uintptr_t)&SLCR->UART_CLK_CTRL;
        case PERIPH_SPI0:   return (uintptr_t)&SLCR->SPI_CLK_CTRL;
        case PERIPH_SPI1:   return (uintptr_t)&SLCR->SPI_CLK_CTRL;
        case PERIPH_CAN0:   return (uintptr_t)&SLCR->CAN_CLK_CTRL;
        case PERIPH_CAN1:   return (uintptr_t)&SLCR->CAN_CLK_CTRL;
        case PERIPH_DBG:    return (uintptr_t)&SLCR->DBG_CLK_CTRL;
        case PERIPH_PCAP:   return (uintptr_t)&SLCR->PCAP_CLK_CTRL;
        case PERIPH_FPGA0:  return (uintptr_t)&SLCR->FPGA0_CLK_CTRL;
        case PERIPH_FPGA1:  return (uintptr_t)&SLCR->FPGA1_CLK_CTRL;
        case PERIPH_FPGA2:  return (uintptr_t)&SLCR->FPGA2_CLK_CTRL;
        case PERIPH_FPGA3:  return (uintptr_t)&SLCR->FPGA3_CLK_CTRL;
        default: return 0;
    }
}

static int periph_clk_ctrl_enable_bitpos(enum zynq_periph periph)
{
    switch (periph) {
        case PERIPH_SDIO1:
        case PERIPH_UART1:
        case PERIPH_SPI1:
        case PERIPH_CAN1:
            return 1;
        case PERIPH_FPGA0:
        case PERIPH_FPGA1:
        case PERIPH_FPGA2:
        case PERIPH_FPGA3:
            return -1; // enable bit is more complicated on fpga
        default:
            // most peripherals have the enable bit in bit0
            return 0;
    }
}

static uint periph_clk_ctrl_divisor_count(enum zynq_periph periph)
{
    switch (periph) {
        case PERIPH_GEM0:
        case PERIPH_GEM1:
        case PERIPH_CAN0:
        case PERIPH_CAN1:
        case PERIPH_FPGA0:
        case PERIPH_FPGA1:
        case PERIPH_FPGA2:
        case PERIPH_FPGA3:
            return 2;
        default:
            // most peripherals have a single divisor
            return 1;
    }
}

static const char *periph_to_name(enum zynq_periph periph)
{
    switch (periph) {
        case PERIPH_USB0: return "USB0";
        case PERIPH_USB1: return "USB1";
        case PERIPH_GEM0: return "GEM0";
        case PERIPH_GEM1: return "GEM1";
        case PERIPH_SMC: return "SMC";
        case PERIPH_LQSPI: return "LQSPI";
        case PERIPH_SDIO0: return "SDIO0";
        case PERIPH_SDIO1: return "SDIO1";
        case PERIPH_UART0: return "UART0";
        case PERIPH_UART1: return "UART1";
        case PERIPH_SPI0: return "SPI0";
        case PERIPH_SPI1: return "SPI1";
        case PERIPH_CAN0: return "CAN0";
        case PERIPH_CAN1: return "CAN1";
        case PERIPH_DBG: return "DBG";
        case PERIPH_PCAP: return "PCAP";
        case PERIPH_FPGA0: return "FPGA0";
        case PERIPH_FPGA1: return "FPGA1";
        case PERIPH_FPGA2: return "FPGA2";
        case PERIPH_FPGA3: return "FPGA3";
        default: return "unknown";
    }
}

status_t zynq_set_clock(enum zynq_periph periph, bool enable, enum zynq_clock_source source, uint32_t divisor, uint32_t divisor2)
{
    DEBUG_ASSERT(periph < _PERIPH_MAX);
    DEBUG_ASSERT(!enable || (divisor > 0 && divisor <= 0x3f));
    DEBUG_ASSERT(source < 4);

    // get the clock control register base
    addr_t clk_reg = periph_clk_ctrl_reg(periph);
    DEBUG_ASSERT(clk_reg != 0);

    int enable_bitpos = periph_clk_ctrl_enable_bitpos(periph);

    zynq_slcr_unlock();

    // if we're enabling
    if (enable) {
        uint32_t ctrl = *REG32(clk_reg);

        // set the divisor, divisor2 (if applicable), source, and enable
        ctrl = (ctrl & ~(0x3f << 20)) | (divisor2 << 20);
        ctrl = (ctrl & ~(0x3f << 8)) | (divisor << 8);
        ctrl = (ctrl & ~(0x3 << 4)) | (source << 4);

        if (enable_bitpos >= 0)
            ctrl |= (1 << enable_bitpos);

        *REG32(clk_reg) = ctrl;
    } else {
        if (enable_bitpos >= 0) {
            // disabling
            uint32_t ctrl = *REG32(clk_reg);

            ctrl &= ~(1 << enable_bitpos);

            *REG32(clk_reg) = ctrl;
        }
    }

    zynq_slcr_lock();

    return NO_ERROR;
}

uint32_t zynq_get_clock(enum zynq_periph periph)
{
    DEBUG_ASSERT(periph < _PERIPH_MAX);

    // get the clock control register base
    addr_t clk_reg = periph_clk_ctrl_reg(periph);
    DEBUG_ASSERT(clk_reg != 0);

    int enable_bitpos = periph_clk_ctrl_enable_bitpos(periph);

    LTRACEF("clkreg 0x%x\n", *REG32(clk_reg));

    // see if it's enabled
    if (enable_bitpos >= 0) {
        if ((*REG32(clk_reg) & (1 << enable_bitpos)) == 0) {
            // not enabled
            return 0;
        }
    }

    // get the source clock
    uint32_t srcclk;
    switch (BITS_SHIFT(*REG32(clk_reg), 5, 4)) {
        case 0: case 1:
            srcclk = get_io_pll_freq();
            break;
        case 2:
            srcclk = get_arm_pll_freq();
            break;
        case 3:
            srcclk = get_ddr_pll_freq();
            break;
    }

    // get the divisor out of the register
    uint32_t divisor = BITS_SHIFT(*REG32(clk_reg), 13, 8);
    if (divisor == 0)
        return 0;

    uint32_t divisor2 = 1;
    if (periph_clk_ctrl_divisor_count(periph) == 2) {
        divisor2 = BITS_SHIFT(*REG32(clk_reg), 25, 20);
        if (divisor2 == 0)
            return 0;
    }

    uint32_t clk = srcclk / divisor / divisor2;

    return clk;
}

void zynq_dump_clocks(void)
{
    printf("zynq clocks:\n");
    printf("\tarm pll %d\n", get_arm_pll_freq());
    printf("\tddr pll %d\n", get_ddr_pll_freq());
    printf("\tio  pll %d\n", get_io_pll_freq());

    printf("\tarm clock %d\n", zynq_get_arm_freq());
    printf("\tarm timer clock %d\n", zynq_get_arm_timer_freq());
    printf("\tcpu6x4x clock %d\n", get_cpu_6x4x_freq());
    printf("\tcpu3x2x clock %d\n", get_cpu_3x2x_freq());
    printf("\tcpu2x clock %d\n", get_cpu_2x_freq());
    printf("\tcpu1x clock %d\n", get_cpu_1x_freq());

    printf("peripheral clocks:\n");
    for (uint i = 0; i < _PERIPH_MAX; i++) {
        printf("\tperiph %d (%s) clock %u\n", i, periph_to_name(i), zynq_get_clock(i));
    }
}

