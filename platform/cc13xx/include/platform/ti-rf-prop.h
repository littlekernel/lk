/*
 * Copyright (c) 2016 Brian Swetland
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

// Standard Packet Format
// [ Preamble ][ Sync Word ][ Length Field ][ Address ][ Payload ][ CRC ]
// 0-30 bytes  0-32 bytes   opt byte        opt byte   0-255      opt 16bit

#define CMD_PROP_TX			0x3801
#define CMD_PROP_RX			0x3802
#define CMD_PROP_TX_ADV			0x3803
#define CMD_PROP_RX_ADV			0x3804
#define CMD_PROP_CS			0x3805 // cc13xx only
#define CMD_PROP_RADIO_SETUP		0x3806 // cc26xx only
#define CMD_PROP_RADIO_DIV_SETUP	0x3807 // cc13xx only

#define CMD_PROP_SET_LEN		0x3401
#define CMD_PROP_RESTART_RX		0x3402

#define PROP_DONE_OK			0x3400
#define PROP_DONE_RXTIMEOUT		0x3401
#define PROP_DONE_BREAK			0x3402 // tx abort due to timeout
#define PROP_DONE_ENDED			0x3403 // rx end trigger
#define PROP_DONE_STOPPED		0x3404 // stopped by CMD_STOP
#define PROP_DONE_ABORT			0x3405 // stopped by CMD_ABORT
#define PROP_DONE_RXERR			0x3406 // crc error
#define PROP_DONE_IDLE			0x3407 // CS ended because idle (cc13xx only)
#define PROP_DONE_BUSY			0x3408 // CS ended because busy (cc13xx only)
#define PROP_DONE_IDLETIMEOUT		0x3409 // CS (cc13xx only)
#define PROP_DONE_BUSYTIMEOUT		0x3409 // CS (cc13xx only)

#define PROP_ERROR_PAR			0x3800 // illegal parameter
#define PROP_ERROR_TXBUF		0x3801 // no available tx buffer at sop
#define PROP_ERROR_RXFULL		0x3802 // out of rx buffers during rx
#define PROP_ERROR_NO_SETUP		0x3803 // radio not in proprietary mode
#define PROP_ERROR_NO_FS		0x3804 // freq synth was off
#define PROP_ERROR_RXOVF		0x3805 // rx overflow
#define PROP_ERROR_TXUNF		0x3806 // tx underflow

typedef struct rf_op_prop_tx rf_op_prop_tx_t;
typedef struct rf_op_prop_tx_adv rf_op_prop_tx_adv_t;
typedef struct rf_op_prop_rx rf_op_prop_rx_t;
typedef struct rf_prop_output rf_prop_output_t;

#define PROP_TX_FS_ON		(0 << 0) // leave freq synth on after
#define PROP_TX_FS_OFF		(1 << 0) // turn freq synth off after
#define PROP_TX_USE_CRC		(1 << 3) // append CRC to packet
#define PROP_TX_VAR_LEN		(1 << 4) // send pkt_len as first byte

struct rf_op_prop_tx {
	uint16_t cmd;
	uint16_t status;
	void *next_op;
	uint32_t start_time;
	uint8_t start_trig;
	uint8_t cond;

	uint8_t config;
	uint8_t pkt_len;
	uint32_t sync_word;
	void *data;
};

#define PROP_TXA_FS_ON		(0 << 0) // leave freq synth on after
#define PROP_TXA_FS_OFF		(1 << 0) // turn off freq synth after
#define PROP_TXA_USE_CRC	(1 << 3) // append crc to packet
#define PROP_TXA_CRC_INC_SW	(1 << 4) // include sync word in crc calc
#define PROP_TXT_CRC_INC_HDR	(1 << 5) // include header in crc calc

struct rf_op_prop_tx_adv {
	uint16_t cmd;
	uint16_t status;
	void *next_op;
	uint32_t start_time;
	uint8_t start_trig;
	uint8_t cond;

	uint8_t config;
	uint8_t num_hdr_bits; // 0-32
	uint16_t pkt_len; // 0 = unlimited
	uint8_t start_conf; // 0
	uint8_t pre_trig; // trigger for preamble->sync, NOW = one preamble
	uint32_t pre_time;
	uint32_t sync_word;
	void *data; // packet data, or TX queue for unlimited length
};

#define PROP_RX_FS_ON		(0 << 0) // leave freq synth on after
#define PROP_RX_FS_OFF		(1 << 0) // turn off freq synth after
#define PROP_RX_REPEAT_OK	(1 << 1) // continue receiving after success
#define PROP_RX_REPEAT_NOT_OK	(1 << 2) // continue receiving after failure
#define PROP_RX_USE_CRC		(1 << 3) // check crc
#define PROP_RX_VAR_LEN		(1 << 4) // first byte is packet length
#define PROP_RX_CHECK_ADDRESS	(1 << 5)
#define PROP_RX_END_STOP	(1 << 6) // packet discarded if end trg during rx
#define PROP_RX_KEEP_BAD_ADDR	(1 << 7) // receive (but mark ignored) if addr mismatch

#define PROP_RX_AUTOFLUSH_IGNORED	(1 << 0) // discard ignored packets
#define PROP_RX_AUTOFLUSH_CRC_ERR	(1 << 1) // discard CRC error packets
#define PROP_RX_INC_HDR			(1 << 3) // include header byte in rxdata
#define PROP_RX_INC_CRC			(1 << 4) // include crc field in rxdata
#define PROP_RX_INC_RSSI		(1 << 5) // include rssi byte
#define PROP_RX_INC_TIMESTAMP		(1 << 6) // include timestamp word
#define PROP_RX_INC_STATUS		(1 << 7) // include status byte

#define PROP_STATUS_ADDR_INDEX_MASK	0x1F
#define PROP_STATUS_ALT_SYNC_WORD	0x20
#define PROP_STATUS_MASK		0xC0
#define PROP_STATUS_RX_OK		0x00
#define PROP_STATUS_CRC_ERR		0x40
#define PROP_STATUS_IGNORE		0x80
#define PROP_STATUS_EX_ABORTED		0xC0

struct rf_op_prop_rx {
	uint16_t cmd;
	uint16_t status;
	void *next_op;
	uint32_t start_time;
	uint8_t start_trig;
	uint8_t cond;

	uint8_t config;
	uint8_t rx_config;
	uint32_t sync_word;
	uint8_t max_pkt_len; // 0 = unknown/unlimited
	uint8_t addr0;
	uint8_t addr1;
	uint8_t end_trig;
	uint32_t end_time;
	rf_queue_t *queue;
	rf_prop_output_t *output;
};

struct rf_prop_output {
	uint16_t num_rx_ok;
	uint16_t num_rx_err;
	uint8_t num_rx_ignored; // ignored due to addr mismatch
	uint8_t num_rx_stopped; // rx fail due to addr mismatch or bad length
	uint8_t num_rx_full; // discarded due to lack of buffer space
	uint8_t last_rssi; // rssi at last sync word match
	uint32_t timestamp; // of last rx'd packet
};

STATIC_ASSERT(sizeof(rf_op_prop_tx_t) == 24);
STATIC_ASSERT(sizeof(rf_op_prop_tx_adv_t) == 32);
STATIC_ASSERT(sizeof(rf_op_prop_rx_t) == 36);
//STATIC_ASSERT(sizeof(rf_op_prop_rx_adv_t) == 48);
STATIC_ASSERT(sizeof(rf_prop_output_t) == 12);
