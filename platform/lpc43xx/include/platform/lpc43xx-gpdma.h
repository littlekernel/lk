/*
 * Copyright (c) 2015 Brian Swetland
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

// these are bitmasks of ch0..ch7 in bit0..bit7
#define DMA_INTSTAT		0x40002000 // ro: INTTCSTAT | INTERRSTAT
#define DMA_INTTCSTAT		0x40002004
#define DMA_INTTCCLR		0x40002008 // wtc
#define DMA_INTERRSTAT		0x4000200C
#define DMA_INTERRCLR		0x40002010 // wtc
#define DMA_INTTCRAW		0x40002014 // not masked
#define DMA_INTERRRAW		0x40002018 // not masked
#define DMA_ENABLED		0x4000201C

#define DMA_CONFIG		0x40002030
#define DMA_CONFIG_EN		(1 << 0) // enable controller
#define DMA_CONFIG_M0_BE	(1 << 1) // master0 big-endian mode
#define DMA_CONFIG_M1_BE	(1 << 2) // master1 big-endian mode

#define DMA_SRC(n)		(0x40002100 + ((n) * 0x20))
#define DMA_DST(n)		(0x40002104 + ((n) * 0x20))
#define DMA_LLI(n)		(0x40002108 + ((n) * 0x20))
#define DMA_CTL(n)		(0x4000210C + ((n) * 0x20))
#define DMA_CFG(n)		(0x40002110 + ((n) * 0x20))

// DMA_CTL bits
#define BURST_1			0
#define BURST_4			1
#define BURST_8			2
#define BURST_16		3
#define BURST_32		4
#define BURST_64		5
#define BURST_128		6
#define BURST_256		7

#define DMA_XFER_SIZE(n)	((n) & 0xFFF)
#define DMA_SRC_BURST(sz)	(((sz) & 7) << 12)
#define DMA_DST_BURST(sz)	(((sz) & 7) << 15)
#define DMA_SRC_BYTE		(0 << 18)
#define DMA_SRC_HALF		(1 << 18)
#define DMA_SRC_WORD		(2 << 18)
#define DMA_DST_BYTE		(0 << 21)
#define DMA_DST_HALF		(1 << 21)
#define DMA_DST_WORD		(2 << 21)
#define DMA_SRC_MASTER0		(0 << 24)
#define DMA_SRC_MASTER1		(1 << 24)
#define DMA_DST_MASTER0		(0 << 25) // memory only
#define DMA_DST_MASTER1		(1 << 25)
#define DMA_SRC_INCR		(1 << 26)
#define DMA_DST_INCR		(1 << 27)
#define DMA_PROT1		(1 << 28) // Privileged
#define DMA_PROT2		(1 << 29) // Bufferable
#define DMA_PROT3		(1 << 30) // Cacheable
#define DMA_TC_IE		(1 << 31) // enable irq on terminal count

// DMA_CFG bits
#define DMA_ENABLE		(1 << 0)
#define DMA_SRC_PERIPH(n)	(((n) & 15) << 1)
#define DMA_DST_PERIPH(n)	(((n) & 15) << 6)
#define DMA_FLOW_M2M_DMAc	(0 << 11) // dma ctl
#define DMA_FLOW_M2P_DMAc	(1 << 11) // dma ctl
#define DMA_FLOW_P2M_DMAc	(2 << 11) // dma ctl
#define DMA_FLOW_P2P_DMAc	(3 << 11) // dma ctl
#define DMA_FLOW_P2P_DPc	(4 << 11) // dst per ctl
#define DMA_FLOW_M2P_DPc	(5 << 11) // dst per ctl
#define DMA_FLOW_P2M_SPc	(6 << 11) // src per ctl
#define DMA_FLOW_P2P_SPc	(7 << 11) // src per ctl
#define DMA_ERR_IRQ_EN		(1 << 14)
#define DMA_TC_IRQ_EN		(1 << 15)
#define DMA_LOCK_XFER		(1 << 16)
#define DMA_ACTIVE		(1 << 17) // ro: data in fifo
#define DMA_HALT		(1 << 18) // ignore source dreqs, drain fifo


#define DMAMUX_REG	0x4004311C

#define DMAMUX_P(n,v)	(((v) & 3) << (((n) & 15) << 1))
#define DMAMUX_M(n)	(~DMAMUX_P(n,3))

#define P0_SPIFI	0
#define P0_SCT_OUT2	1
#define P0_SGPIO14	2
#define P0_TIMER3_M1	3

#define P1_TIMER0_M0	0
#define P1_USART0_TX	1
#define P1_AES_IN	3

#define P2_TIMER0_M1	0
#define P2_USART0_RX	1
#define P2_AES_OUT	3

#define P3_TIMER1_M0	0
#define P3_UART1_TX	1
#define P3_I2S1_DMA1	2
#define P3_SSP1_TX	3

#define P4_TIMER1_M1	0
#define P4_UART1_RX	1
#define P4_I2S1_DMA2	2
#define P4_SSP1_RX	3

#define P5_TIMER2_M0	0
#define P5_USART2_TX	1
#define P5_SSP1_TX	2
#define P5_SGPIO15	3

#define P6_TIMER2_M1	0
#define P6_USART2_RX	1
#define P6_SSP1_RX	2
#define P6_SGPIO14	3

#define P7_TIMER3_M0	0
#define P7_USART3_TX	1
#define P7_SCT_DMA0	2
#define P7_ADCHS_WR	3

#define P8_TIMER3_M1	0
#define P8_USART3_RX	1
#define P8_SCT_DMA1	2
#define P8_ADCHS_RD	3

#define P9_SSP0_RX	0
#define P9_I2S0_DMA1	1
#define P9_SCT_DMA1	2

#define P10_SSP0_RX	0
#define P10_I2S0_DMA2	1
#define P10_SCT_DMA0	2

#define P11_SSP1_RX	0
#define P11_SGPIO14	1
#define P11_USART0_TX	2

#define P12_SSP1_RX	0
#define P12_SGPIO15	1
#define P12_USART0_RX	2

#define P13_ADC0	0
#define P13_AES_IN	1
#define P13_SSP1_RX	2
#define P13_USART3_RX	3

#define P14_ADC1	0
#define P14_AES_OUT	1
#define P14_SSP1_TX	2
#define P14_USART3_TX	3

#define P15_DAC		0
#define P15_SCT_OUT3	1
#define P15_SGPIO15	2
#define P15_TIMER3_M0	3

