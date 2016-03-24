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

#define SPIFI_CTRL		0x40003000 // Control
#define SPIFI_CMD		0x40003004 // Command
#define SPIFI_ADDR		0x40003008 // Address
#define SPIFI_IDATA		0x4000300C // Intermediate Data
#define SPIFI_CLIMIT		0x40003010 // Cache Limit
#define SPIFI_DATA		0x40003014 // Data
#define SPIFI_MCMD		0x40003018 // Memory Command
#define SPIFI_STAT		0x4000301C // Status

#define CTRL_TIMEOUT(n)		((n) & 0xFFFF)
#define CTRL_CSHIGH(n)		(((n) & 0xF) << 16) // Minimum /CS high time (serclks - 1)
#define CTRL_D_PRFTCH_DIS	(1 << 21) // Disable Prefetch of Data
#define CTRL_INTEN		(1 << 22) // Enable IRQ on end of command
#define CTRL_MODE3		(1 << 23) // 0=SCK low after +edge of last bit, 1=high
#define CTRL_PRFTCH_DIS		(1 << 27) // Disable Prefetch
#define CTRL_DUAL		(1 << 28) // 0=Quad 1=Dual (bits in "wide" ops)
#define CTRL_QUAD		(0 << 28)
#define CTRL_RFCLK		(1 << 29) // 1=sample read data on -edge clock
#define CTRL_FBCLK		(1 << 30) // use feedback clock from SCK pin for sampling
#define CTRL_DMAEN		(1 << 31) // enable DMA request output

#define CMD_DATALEN(n)		((n) & 0x3FFF)
#define CMD_POLL		(1 << 14) // if set, read byte repeatedly until condition
#define CMD_POLLBIT(n)		((n) & 7) // which bit# to check
#define CMD_POLLSET		(1 << 3)  // condition is bit# set
#define CMD_POLLCLR		(0 << 3)  // condition is bit# clear
#define CMD_DOUT		(1 << 15) // 1=data phase output, 0=data phase input
#define CMD_DIN			(0 << 15)
#define CMD_INTLEN(n)		(((n) & 7) << 16) // count of intermediate bytes
#define CMD_FF_SERIAL		(0 << 19) // all command fields serial
#define CMD_FF_WIDE_DATA	(1 << 19) // data is wide, all other fields serial
#define CMD_FF_SERIAL_OPCODE	(2 << 19) // opcode is serial, all other fields wide
#define CMD_FF_WIDE		(3 << 19) // all command fields wide
#define CMD_FR_OP		(1 << 21) // frame format: opcode only
#define CMD_FR_OP_1B		(2 << 21) // opcode, lsb addr
#define CMD_FR_OP_2B		(3 << 21) // opcode, 2 lsb addr
#define CMD_FR_OP_3B		(4 << 21) // opcode, 3 lsb addr
#define CMD_FR_OP_4B		(5 << 21) // opcode, 4b address
#define CMD_FR_3B		(6 << 21) // 3 lsb addr
#define CMD_FR_4B		(7 << 21) // 4 lsb addr
#define CMD_OPCODE(n)		((n) << 24)

// MCMD register defines CMD for automatic reads
// Similar to CMD, but
// DATALEN, POLL, DOUT must be 0

#define STAT_MCINIT		(1 << 0) // set on sw write to MCMD, clear on RST, wr(0)
#define STAT_CMD		(1 << 1) // set when CMD written, clear on CS, RST
#define STAT_RESET		(1 << 4) // write 1 to abort current txn or memory mode
#define STAT_INTRQ		(1 << 5) // read IRQ status, wr(1) to clear

// 1. Write ADDR
// 2. Write CMD
// 3. Read/Write DATA necessary number of times to transfer DATALEN bytes
//    (byte/half/word ops move 1/2/4 bytes at a time)

