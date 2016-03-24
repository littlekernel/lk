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

#define UART0_BASE	0x40081000
#define UART1_BASE	0x40082000
#define UART2_BASE	0x400C1000
#define UART3_BASE	0x400C2000

#define REG_RBR		0x00 // RO Recv Buffer       (DLAB==0)
#define REG_THR		0x00 // WO Xmit Holding      (DLAB==0)
#define REG_IER		0x04 // RW Interrupt Enable  (DLAB==0)
#define REG_DLL		0x00 // RW Divisor Latch LSB (DLAB==1)
#define REG_DLM		0x04 // RW Divisor Latch MSB (DLAB==1)
#define REG_IIR		0x08 // RO Interrupt ID
#define REG_FCR		0x08 // WO Fifo Control
#define REG_LCR		0x0C // RW Line Control
#define REG_MCR		0x10 // RW Modem Control (UART1 only)
#define REG_LSR		0x14 // RO Line Status
#define REG_MSR		0x18 // RO Modem Status (UART1 only)
#define REG_SCR		0x1C // RW Scratcpad (no hw use)
#define REG_ACR		0x20 // RW Auto-baud Control
#define REG_ICR		0x24 // RW IrDA Control
#define REG_FDR		0x28 // RW Fractional Divider
#define REG_OSR		0x2C // RW Oversampling (REG0/2/3 only)

#define IER_RBRIE	(1 << 0) // enable receive data avail
#define IER_THREIE	(1 << 1) // enable THRE irq
#define IER_RXIE	(1 << 2) // enable RX Line Status IRQs

#define IIR_INTSTATUS	(1 << 0) // 0=IRQ Pending
#define IIR_INTID_MASK	(3 << 1)
#define IIR_INTID_RLS	(3 << 1) // Receive Line Status
				 // Cleared on LSR Read
#define IIR_INTID_RDA	(2 << 1) // Receive Data Available
				 // Cleared when FIFO < trigger level
#define IIR_INTID_CTI	(6 << 1) // Character Timeout
				 // data in FIFO, and 3.5-4.5 char times idle
#define IIR_INTID_THRE	(1 << 1) // Transmit Holding Register Empty
#define IIR_INTID_NONE	(0 << 1)

#define FCR_FIFOEN	(1 << 0) // enable FIFO
#define FCR_RXFIFORES	(1 << 1) // RX FIFO reset
#define FCR_TXFIFORES	(1 << 2) // TX FIFO reset
#define FCR_DMAMODE	(1 << 3) // select DMA mode
#define FCR_RX_TRIG_1	(0 << 6) // RX Trigger at 1 byte
#define FCR_RX_TRIG_4	(1 << 6) // RX Trigger at 4 bytes
#define FCR_RX_TRIG_8	(2 << 6) // RX Trigger at 8 bytes
#define FCR_RX_TRIG_14	(3 << 6) // RX Trigger at 14 bytes

#define LCR_WLS_5	(0 << 0) // 5bit character
#define LCR_WLS_6	(1 << 0) // 6bit character
#define LCR_WLS_7	(2 << 0) // 7bit character
#define LCR_WLS_8	(3 << 0) // 8bit character
#define LCR_SBS_1	(0 << 2) // 1 stop bit
#define LCR_SBS_2	(1 << 2) // 2 stop bits
#define LCR_PE		(1 << 3) // parity enable
#define LCR_PS_ODD	(0 << 4) // odd parity
#define LCR_PS_EVEN	(1 << 4) // even parity
#define LCR_PS_HIGH	(2 << 4) // always-1 parity
#define LCR_PS_LOW	(3 << 4) // always-0 parity
#define LCR_BC		(1 << 6) // enable break transmission
#define LCR_DLAB	(1 << 7) // enable access to divisor latches

#define LSR_RDR		(1 << 0) // receiver data ready
#define LSR_OE		(1 << 1) // overrun error (fifo was full, character lost)
#define LSR_PE		(1 << 2) // parity error (top of fifo)
#define LSR_FE		(1 << 3) // framing error (top of fifo)
#define LSR_BI		(1 << 4) // break interrupt
#define LSR_THRE	(1 << 5) // transmit holding register empty
#define LSR_TEMT	(1 << 6) // transmitter empty
#define LSR_RXFE	(1 << 7) // error in RX FIFO
#define LSR_TXERR	(1 << 8) // NACK received in smart card mode

#define FDR_DIVADDVAL(n) ((n) & 0xF)
#define FDR_MULVAL(n) (((n) & 0xF) << 4)

// baud rate selection:
//
// PCLK / ( 16 * ( 256 * DLM + DLL ) * ( 1 + ( DivAddVal / MulVal ) )
//
// 1 <= MulVal <= 15            DivAddVal == 0  -> Disables Frac Divider
// 0 <= DivAddVal <= 14
// DivAddVal < MulVal


#define OSR_OSFRAC(n)	(((n) & 0x3) << 1) // fractional part
#define OSR_OSINT(n)	(((n) & 0xF) << 4) // integer part - 1

// oversampling rate = OsInt + 1 + ( 1/8 * OsFrac)   (default is 16)


