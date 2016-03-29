/* rswdp.h - remote serial wire debug protocol
 *
 * Copyright 2011 Brian Swetland <swetland@frotz.net>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* Remote Serial Wire Debug Protocol */

#ifndef _RSWDP_PROTOCOL_H_
#define _RSWDP_PROTOCOL_H_

/* Basic framing:
 * - host and device exchange "transactions" consisting of
 *   some number of "messages".
 * - each "message" has a 32bit header and may have 0 or more
 *   32bit words of payload
 * - a transaction may not exceed 4K (1024 words)
 * - a transaction is sent in a series of USB BULK packets
 * - the final packet must be a short packet unless the
 *   transaction is exactly 4K in length
 * - packets must be a multiple of 4 bytes
 * - the first message in a transaction must be
 *   CMD_TXN_START or CMD_TXN_ASYNC
 */

#define RSWD_MSG(cmd,op,n)	((((cmd)&0xFF) << 24) | (((op) & 0xFF)<<16) | ((n) & 0xFFFF))
#define RSWD_MSG_CMD(n)		(((n) >> 24) & 0xFF)
#define RSWD_MSG_OP(n)		(((n) >> 16) & 0xFF)
#define RSWD_MSG_ARG(n)		((n) & 0xFFFF)

#define RSWD_TXN_START(seq)	(0xAA770000 | ((seq) & 0xFFFF))
#define RSWD_TXN_ASYNC		(0xAA001111)

/* valid: either */
#define CMD_NULL	0x00 /* used for padding */

/* valid: host to target */
#define CMD_SWD_WRITE	0x01 /* op=addr arg=count payload: data x count */
#define CMD_SWD_READ	0x02 /* op=addr arg=count payload: data x count */
#define CMD_SWD_DISCARD	0x03 /* op=addr arg=count payload: none (discards) */
#define CMD_ATTACH	0x04 /* do swdp reset/connect handshake */
#define CMD_RESET	0x05 /* arg=1 -> assert RESETn, otherwise deassert */
#define CMD_DOWNLOAD	0x06 /* arg=wordcount, payload: addr x 1, data x n */
#define CMD_EXECUTE	0x07 /* payload: addr x 1 */
#define CMD_TRACE	0x08 /* op=tracebits n=0 */
#define CMD_BOOTLOADER	0x09 /* return to bootloader for reflashing */
#define CMD_SET_CLOCK	0x0A /* set SWCLK rate to n khz */
#define CMD_SWO_CLOCK	0x0B /* set SWOCLK rate to n khz, 0 = disable SWO */
#define CMD_JTAG_IO	0x0C /* op=0, arg=bitcount, data x (count/32) * 2 */
			     /* tms, tdi word pairs per 32bits */
#define CMD_JTAG_TX	0x0D /* tms=op.0, arg=bitcount, data x (count/32) words of tdi */
#define CMD_JTAG_RX	0x0E /* tms=op.0, tdi=op.1, arg=bitcount, return (count/32) words */
#define CMD_JTAG_VRFY	0x0F /* arg=bitcount, tms/tdi data/mask, error if tdo&mask != data */

/* valid: target to host */
#define CMD_STATUS	0x10 /* op=errorcode, arg=commands since last TXN_START */
#define CMD_SWD_DATA	0x11 /* op=0 arg=count, payload: data x count */
#define CMD_SWO_DATA	0x12 /* op=0 arg=count, payload: ((count+3)/4) words */
#define CMD_JTAG_DATA	0x14 /* op=0 arg=bitcount, payload: (arg/32) words */

/* valid: target to host async */
#define CMD_DEBUG_PRINT	0x20 /* arg*4 bytes of ascii debug output */

/* valid: bidirectional query/config messages */
#define CMD_VERSION	0x30 /* arg=bcdversion (0x0100 etc) */
#define CMD_BUILD_STR	0x31 /* arg=wordcount, payload = asciiz */
#define CMD_BOARD_STR	0x32 /* arg=wordcount, payload = asciiz */
#define CMD_RX_MAXDATA	0x33 /* arg=bytes, declares senders rx buffer size */
#define CMD_CLOCK_KHZ	0x34 /* arg=khz, reports active clock rate */

/* CMD_STATUS error codes */
#define ERR_NONE	0
#define ERR_INTERNAL	1
#define ERR_TIMEOUT	2
#define ERR_IO		3
#define ERR_PARITY	4
#define ERR_BAD_MATCH	5

#define RSWD_VERSION		0x0102

#define RSWD_VERSION_1_0	0x0100
#define RSWD_VERSION_1_1	0x0101
#define RSWD_VERSION_1_2	0x0102

// Pre-1.0
//  - max packet size fixed at 2048 bytes
//
// Version 1.0
// - CMD_VERSION, CMD_BUILD_STR, CMD_BOARD_STR, CMD_RX_MAXDATA,
//   CMD_CLOCK_KHZ added
//
// Version 1.1
// - CMD_SWO_DATA arg is now byte count, not word count
//
// Version 1.2
// - CMD_JTAG_IO, CMD_JTAG_RX, CMD_JTAG_TX, CMD_JTAG_VRFY, CMD_JTAG_DATA added

/* CMD_SWD_OP operations - combine for direct AP/DP io */
#define OP_RD 0x00
#define OP_WR 0x01
#define OP_DP 0x00
#define OP_AP 0x02
#define OP_X0 0x00
#define OP_X4 0x04
#define OP_X8 0x08
#define OP_XC 0x0C

/* DP registers */
#define DP_IDCODE	(OP_DP|OP_X0)
#define DP_ABORT	(OP_DP|OP_X0)
#define DP_DPCTRL	(OP_DP|OP_X4)
#define DP_RESEND	(OP_DP|OP_X8)
#define DP_SELECT	(OP_DP|OP_X8)
#define DP_BUFFER	(OP_DP|OP_XC)

#endif
