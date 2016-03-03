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

#include <compiler.h>

// trigger control byte (TRM 23.3.2.5.1)

#define TRG_NOW			0x00
#define TRG_NEVER		0x01 // except for CMD_TRIGGER if enabled
#define TRG_ABSTIME		0x02
#define TRG_REL_SUBMIT		0x03 // relative to command submission time
#define TRG_REL_START		0x04 // relative to command start (end trg only)
#define TRG_REL_PREVSTART	0x05 // relative to start of previous command
#define TRG_REL_FIRSTSTART	0x06 // relative to first command in chain
#define TRG_REL_PREVEND		0x07 // relative to end of previous command
#define TRG_REL_EVT1		0x08 // relative to prev command evt1
#define TRG_REL_EVT2		0x09 // relative to prev command evt1
#define TRG_EXTERNAL		0x0A // TRG_EXT_* in timer parameter

#define TRG_ENA_CMD_O		0x10 // CMD_TRIGGER #0 enabled as alt trigger
#define TRG_ENA_CMD_1		0x30 // CMD_TRIGGER #1 enabled as alt trigger
#define TRG_ENA_CMD_2		0x50 // CMD_TRIGGER #2 enabled as alt trigger
#define TRG_ENA_CMD_3		0x70 // CMD_TRIGGER #3 enabled as alt trigger

#define TRG_PAST_OK		0x80 // trigger in the past happens asap
#define TRG_PAST_DISALLOW	0x00 // never happens, or for start trg is an error

#define TRG_EXT_RISING		0x00
#define TRG_EXT_FALLING		0x40
#define TRG_EXT_BOTH_EDGE	0x80
#define TRG_SRC_RFC_GPI0	(22 << 8)
#define TRG_SRC_RFC_GPI1	(23 << 8)

// condition byte (TRM 23.3.2.5.2)
// - commands return TRUE, FALSE, or ABORT as defined for each command
// - a skip of 0 = re-exec current, 1 = exec next, 2 = skip next
#define CND_ALWAYS		0x00
#define CND_NEVER		0x01
#define CND_STOP_ON_FALSE	0x02
#define CND_STOP_ON_TRUE	0x03
#define CND_SKIP_ON_FALSE	0x04 // if false, skip N commands
#define CND_SKIP_ON_TRUE	0x05 // if true, skip N commands
#define CND_SKIP(n)		(((n) & 0xF) << 4)


#define QE_STATUS_PENDING	0x00 // set before submitting by SysCPU
#define QE_STATUS_ACTIVE	0x01 // entry in queue by RadioCPU
#define QE_STATUS_BUSY		0x02 // entry is actively being r/w by RadioCPU
#define QE_STATUS_DONE		0x03 // RadioCPU is done, SysCPU may reclaim

#define QE_CONFIG_GENERAL	0x00
#define QE_CONFIG_MULTI		0x01
#define QE_CONFIG_POINTER	0x02
#define QE_CONFIG_LEN_SZ_0	0x00 // no length prefix
#define QE_CONFIG_LEN_SZ_1	0x04 // 1-byte length prefix
#define QE_CONFIG_LEN_SZ_2	0x08 // 2-byte length prefix

typedef struct rf_queue rf_queue_t;
typedef struct rf_queue_entry rf_queue_entry_t;

struct rf_queue {
	rf_queue_entry_t *curr;
	rf_queue_entry_t *last;
};

struct rf_queue_entry {
	rf_queue_entry_t *next;
	uint8_t status;
	uint8_t config;
	uint16_t length;
	union {
		uint8_t data[4];
		uint8_t *ptr;
	};
};

#define IMM_CMD(cmd,arg,ext)	(((cmd) << 16) | \
				((arg & 0xFF) << 8) | \
				((ext & 0x3F) << 2) | \
				0x01)

// direct commands
#define CMD_ABORT		0x0401 // stop asap
#define CMD_STOP		0x0402 // stop once active rx/tx completes
#define CMD_GET_RSSI		0x0403
#define CMD_TRIGGER		0x0404
#define CMD_TRIGGER_N(n)	(0x0404 | (((n) & 3) << 16))
#define CMD_START_RAT		0x0405
#define CMD_PING		0x0406 // no op

// immediate commands
#define CMD_UPDATE_RADIO_SETUP	0x0001
#define CMD_GET_FW_INFO		0x0002
#define CMD_READ_RFREG		0x0601
#define CMD_SET_RAT_CMP		0x000A
#define CMD_SET_RAT_CPT		0x0603
#define CMD_DISABLE_RAT_CH	0x0408
#define CMD_SET_RAT_OUTPUT	0x0604
#define CMD_ARM_RAT_CH		0x0409
#define CMD_DISARM_RAT_CH	0x040A
#define CMD_SET_TX_POWER	0x0010
#define CMD_UPDATE_FS		0x0011
#define CMD_BUS_REQUEST		0x040E
#define CMD_ADD_DATA_ENTRY	0x0005
#define CMD_REMOVE_DATA_ENTRY	0x0006
#define CMD_FLUSH_QUEUE		0x0007
#define CMD_CLEAR_RX		0x0008
#define CMD_REMOVE_PENDING	0x0009

// queued commands
#define CMD_NOP			0x0801 // rf_op_basic_t
#define CMD_FS			0x0803 // rf_op_fs_t
#define CMD_FS_OFF		0x0804 // rf_op_basic_t
#define CMD_RADIO_SETUP		0x0802 // rf_op_radio_setup_t
#define CMD_FS_POWERUP		0x080C // rf_op_fs_power_t
#define CMD_FS_POWERDOWN	0x080D // rf_op_fs_power_t
#define CMD_SYNC_STOP_RAT	0x0809 // rf_op_sync_rat_t
#define CMD_SYNC_START_RAT	0x080A // rf_op_sync_rat_t
#define CMD_COUNT		0x080B // rf_op_count_t
#define CMD_PATTERN_CHECK	0x0813 // rf_op_pattern_check_t

// status
#define DONE_OK			0x0400 // success
#define DONE_COUNTDOWN		0x0401 // count == 0
#define DONE_RXERR		0x0402 // crc error
#define DONE_TIMEOUT		0x0403
#define DONE_STOPPED		0x0404 // stopped by CMD_DONE
#define DONE_ABORT		0x0405 // stopped by CMD_ABORT
#define DONE_FAILED		0x0406

#define ERROR_PAST_START	0x0800 // start trigger is in the past
#define ERROR_START_TRIG	0x0801 // bad trigger parameter
#define ERROR_CONDITION		0x0802 // bad condition parameter
#define ERROR_PAR		0x0803 // invalid parameter (command specific)
#define ERROR_POINTER		0x0804 // invalid pointer to next op
#define ERROR_CMD_ID		0x0805 // bad command id
#define ERROR_WRONG_BG		0x0806 // fg cmd cannot run w/ active bg cmd
#define ERROR_NO_SETUP		0x0807 // tx/rx without radio setup
#define ERROR_NO_FS		0x0808 // tx/rx with freq synth off
#define ERROR_SYNTH_PROG	0x0809 // freq synth calibration failure
#define ERROR_TXUNF		0x080A // tx underflow
#define ERROR_TXOVF		0x080B // rx overflow
#define ERROR_NO_RX		0x080C // no rx data available
#define ERROR_PENDING		0x080D // other commands already pending

typedef struct rf_op_basic rf_op_basic_t;
typedef struct rf_op_radio_setup rf_op_radio_setup_t;
typedef struct rf_op_fs rf_op_fs_t;
typedef struct rf_op_fs_power rf_op_fs_power_t;
typedef struct rf_op_sync_rat rf_op_sync_rat_t;
typedef struct rf_op_count rf_op_count_t;
typedef struct rf_op_pattern_check rf_op_pattern_check_t;
typedef struct rf_op_fw_info rf_op_fw_info_t;

struct rf_op_basic {
	uint16_t cmd;
	uint16_t status;
	void *next_op;
	uint32_t start_time;
	uint8_t start_trig;
	uint8_t cond;
} __PACKED;

struct rf_op_fw_info {
	uint16_t cmd;

	uint16_t version;
	uint16_t free_ram_start;
	uint16_t free_ram_size;
	uint16_t avail_rat_ch;
} __PACKED;

#if 0
// warning - docs / headers disagree
#define RF_MODE_BLE		0x00
#define RF_MODE_802_15_4	0x01
#define RF_MODE_2MBPS_GFSK	0x02
#define RF_MODE_5MBPS_8FSK	0x05
#define RF_MODE_NO_CHANGE	0xFF
#endif

#define RF_CFG_FE_MODE(n)	((n) & 7)
#define RF_CFG_INT_BIAS		(0 << 3)
#define RF_CFG_EXT_BIAS		(1 << 3)
#define RF_CFG_FS_POWERUP	(0 << 10)
#define RF_CFG_FS_NO_POWERUP	(1 << 10)

#define TX_PWR_IB(n)		((n) & 0x3F)
#define TX_PWR_GC(n)		(((n) & 3) << 6)
#define TX_PWR_TEMP_COEFF(n)	(((n) & 0xFF) << 8)

struct rf_op_radio_setup {
	uint16_t cmd;
	uint16_t status;
	void *next_op;
	uint32_t start_time;
	uint8_t start_trig;
	uint8_t cond;

	uint8_t mode;
	uint8_t io_div; // cc13xx (0,2,5,6,10,12,5,30), cc26xx (0,2)
	uint16_t config;
	uint16_t tx_pwr;
	void *reg_override;
} __PACKED;

struct rf_op_fs {
	uint16_t cmd;
	uint16_t status;
	void *next_op;
	uint32_t start_time;
	uint8_t start_trig;
	uint8_t cond;

	uint16_t frequency;
	uint16_t fract_freq;
	uint8_t synth_conf;
	uint8_t reserved;
	uint8_t mid_precal;
	uint8_t kt_precal;
	uint16_t tdc_precal;
};

struct rf_op_fs_power {
	uint16_t cmd;
	uint16_t status;
	void *next_op;
	uint32_t start_time;
	uint8_t start_trig;
	uint8_t cond;

	uint16_t reserved;
	void *reg_override;
};

struct rf_op_sync_rat {
	uint16_t cmd;
	uint16_t status;
	void *next_op;
	uint32_t start_time;
	uint8_t start_trig;
	uint8_t cond;

	uint16_t reserved;
	uint32_t rat0;
};

// - on start, if count == 0 -> ERROR_PARAM (ABORT?), else count--
// - if count > 0, status = DONE_OK, res = TRUE
// - if count == 0, status = DONE_COUNTDOWN, res = FALSE

struct rf_op_count {
	uint16_t cmd;
	uint16_t status;
	void *next_op;
	uint32_t start_time;
	uint8_t start_trig;
	uint8_t cond;

	uint16_t counter;
};

#define PTN_OP_EQ		(0 << 0)
#define PTN_OP_LT		(1 << 0)
#define PTN_OP_GT		(2 << 0)
#define PTN_BYTE_REV		(1 << 2) // 0=LE, 1=BE
#define PTN_BIT_REV		(1 << 3)
#define PTN_SIGN_EXT(n)		(((n) & 31) << 4)
#define PTN_USE_IDX		(1 << 9)
#define PTN_USE_PTR		(0 << 9)

// 1. read word x from pointer or index from start of last committed rx data
// 2. byteswap x, if requested
// 3. bitswap x, if requested
// 4. x = x & mask
// 5. if sign extend != 0, extend that bit through 31
// 6. result = x OP value
// if result TRUE, status = DONE_OK
// if result FALSE, status = DONE_FAILED
// if USE_IDX but no RX data available, status = ERROR_NO_RX, result = ABORT

struct rf_op_pattern_check {
	uint16_t cmd;
	uint16_t status;
	void *next_op;
	uint32_t start_time;
	uint8_t start_trig;
	uint8_t cond;

	uint16_t pattern;
	void *next_op_if_true;
	union {
		void *ptr;
		int32_t idx;
	};
	uint32_t mask;
	uint32_t value;
};

// multi element is <COUNT:2> [ <NEXT-DATA-INDEX:2> <DATA:1> ... ]

STATIC_ASSERT(sizeof(rf_queue_t) == 8);
STATIC_ASSERT(sizeof(rf_queue_entry_t) == 12);
STATIC_ASSERT(sizeof(rf_op_basic_t) == 14);
STATIC_ASSERT(sizeof(rf_op_radio_setup_t) == 24);
STATIC_ASSERT(sizeof(rf_op_fs_t) == 24);
STATIC_ASSERT(sizeof(rf_op_fs_power_t) == 20);
STATIC_ASSERT(sizeof(rf_op_sync_rat_t) == 20);
STATIC_ASSERT(sizeof(rf_op_count_t) == 16);
STATIC_ASSERT(sizeof(rf_op_pattern_check_t) == 32);
STATIC_ASSERT(sizeof(rf_op_fw_info_t) == 10);
