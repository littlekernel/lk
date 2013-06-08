/*
 * Copyright (c) 2009 Corey Tabaka
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
#include <sys/types.h>
#include <err.h>
#include <reg.h>
#include <debug.h>
#include <kernel/thread.h>
#include <platform.h>
#include <platform/interrupts.h>
#include <platform/console.h>
#include <platform/timer.h>
#include <platform/pc.h>
#include "platform_p.h"
#include <arch/x86.h>
#include <lib/cbuf.h>

static inline int i8042_read_data(void)
{
	return inp(I8042_DATA_REG);
}

static inline int i8042_read_status(void)
{
	return inp(I8042_STATUS_REG);
}

static inline void i8042_write_data(int val)
{
	outp(I8042_DATA_REG, val);
}

static inline void i8042_write_command(int val)
{
	outp(I8042_COMMAND_REG, val);
}

/*
 * timeout in milliseconds
 */
#define I8042_CTL_TIMEOUT   500

/*
 * status register bits
 */
#define I8042_STR_PARITY    0x80
#define I8042_STR_TIMEOUT   0x40
#define I8042_STR_AUXDATA   0x20
#define I8042_STR_KEYLOCK   0x10
#define I8042_STR_CMDDAT    0x08
#define I8042_STR_MUXERR    0x04
#define I8042_STR_IBF       0x02
#define I8042_STR_OBF       0x01

/*
 * control register bits
 */
#define I8042_CTR_KBDINT    0x01
#define I8042_CTR_AUXINT    0x02
#define I8042_CTR_IGNKEYLK  0x08
#define I8042_CTR_KBDDIS    0x10
#define I8042_CTR_AUXDIS    0x20
#define I8042_CTR_XLATE     0x40

/*
 * commands
 */
#define I8042_CMD_CTL_RCTR  0x0120
#define I8042_CMD_CTL_WCTR  0x1060
#define I8042_CMD_CTL_TEST  0x01aa

#define I8042_CMD_KBD_DIS   0x00ad
#define I8042_CMD_KBD_EN    0x00ae
#define I8042_CMD_KBD_TEST  0x01ab
#define I8042_CMD_KBD_MODE  0x01f0

/*
 * used for flushing buffers. the i8042 internal buffer shoudn't exceed this.
 */
#define I8042_BUFFER_LENGTH 32

static inline void delay(lk_time_t delay)
{
	lk_time_t start = current_time();

	while (start + delay > current_time());
}

/* scancodes we want to do something with that don't translate via table */
#define SCANCODE_LSHIFT 0x2a
#define SCANCODE_RSHIFT 0x36

/* scancode translation tables */
static const int KeyCodeSingleLower[] = {
// 0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
  -1,  -1, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=','\b','\t', // 0
 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']','\n',  -1, 'a', 's', // 1
 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';','\'', '`',  -1,'\\', 'z', 'x', 'c', 'v', // 2
 'b', 'n', 'm', ',', '.', '/',  -1, '*',  -1, ' ',  -1,  -1,  -1,  -1,  -1,  -1, // 3
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, // 4
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, // 5
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, // 6
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, // 7
};

static const int KeyCodeMultiLower[] = {
// 0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, // 0
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, // 1
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, // 2
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, // 3
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, // 4
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, // 5
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, // 6
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, // 7
};

static const int KeyCodeSingleUpper[] = {
// 0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
  -1,  -1, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+','\b','\t', // 0
 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}','\n',  -1, 'A', 'S', // 1
 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',  -1, '|', 'Z', 'X', 'C', 'V', // 2
 'B', 'N', 'M', '<', '>', '?',  -1, '*',  -1, ' ',  -1,  -1,  -1,  -1,  -1,  -1, // 3
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, // 4
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, // 5
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, // 6
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, // 7
};

static const int KeyCodeMultiUpper[] = {
// 0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, // 0
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, // 1
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, // 2
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, // 3
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, // 4
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, // 5
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, // 6
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, // 7
};

/*
 * state key flags
 */
static bool key_lshift;
static bool key_rshift;

static cbuf_t key_buf;

static void i8042_process_scode(uint8_t scode, unsigned int flags)
{
	static int lastCode = 0;
	int keyCode;
	uint8_t keyUpBit;

	bool multi = lastCode == 0xe0;

	// save the key up event bit
	keyUpBit = scode & 0x80;
	scode &= 0x7f;

	if (scode == SCANCODE_LSHIFT) {
		key_lshift = !keyUpBit;
	}

	if (scode == SCANCODE_RSHIFT) {
		key_rshift = !keyUpBit;
	}

	if (key_lshift || key_rshift) {
		keyCode = multi ? KeyCodeMultiUpper[scode] : KeyCodeSingleUpper[scode];
	} else {
		keyCode = multi ? KeyCodeMultiLower[scode] : KeyCodeSingleLower[scode];
	}

	/*printf_xy(71, 3, BLUE, "%02x%02x %c %c%c", multi ? lastCode : 0, scode,
	    keyCode != -1 ? (char) keyCode : ' ', key_lshift ? 'L' : ' ',
	    key_rshift ? 'R' : ' ');*/

	if (keyCode != -1 && !keyUpBit) {
		char c = (char) keyCode;
		cbuf_write_char(&key_buf, c, false);
	}

	// update the last received code
	lastCode = scode;
}

static int i8042_wait_read(void)
{
	int i = 0;
	while ((~i8042_read_status() & I8042_STR_OBF) && (i < I8042_CTL_TIMEOUT)) {
		delay(1);
		i++;
	}
	return -(i == I8042_CTL_TIMEOUT);
}

static int i8042_wait_write(void)
{
	int i = 0;
	while ((i8042_read_status() & I8042_STR_IBF) && (i < I8042_CTL_TIMEOUT)) {
		delay(1);
		i++;
	}
	return -(i == I8042_CTL_TIMEOUT);
}

static int i8042_flush(void)
{
	unsigned char data __UNUSED;
	int i = 0;

	//enter_critical_section();

	while ((i8042_read_status() & I8042_STR_OBF) && (i++ < I8042_BUFFER_LENGTH)) {
		delay(1);
		data = i8042_read_data();
	}

	//exit_critical_section();

	return i;
}

static int i8042_command(uint8_t *param, int command)
{
	int retval = 0, i = 0;

	//enter_critical_section();

	retval = i8042_wait_write();
	if (!retval) {
		i8042_write_command(command & 0xff);
	}

	if (!retval) {
		for (i = 0; i < ((command >> 12) & 0xf); i++) {
			if ((retval = i8042_wait_write())) {
				break;
			}

			i8042_write_data(param[i]);
		}
	}

	if (!retval) {
		for (i = 0; i < ((command & 0xf0) >> 8); i++) {
			if ((retval = i8042_wait_read())) {
				break;
			}

			if (i8042_read_status() & I8042_STR_AUXDATA) {
				param[i] = ~i8042_read_data();
			} else {
				param[i] = i8042_read_data();
			}
		}
	}

	//exit_critical_section();

	return retval;
}

static enum handler_return i8042_interrupt(void *arg)
{
	uint8_t str, data = 0;

	//enter_critical_section();
	str = i8042_read_status();
	if (str & I8042_STR_OBF) {
		data = i8042_read_data();
	}
	//exit_critical_section();

	if (str & I8042_STR_OBF) {
		i8042_process_scode(data,
		                    ((str & I8042_STR_PARITY) ? I8042_STR_PARITY : 0) |
		                    ((str & I8042_STR_TIMEOUT) ? I8042_STR_TIMEOUT : 0));
	}

	return INT_NO_RESCHEDULE;
}

int platform_read_key(char *c)
{
	ssize_t len;

	len = cbuf_read_char(&key_buf, c, true);
	return len;
}

void platform_init_keyboard(void)
{
	uint8_t ctr;

	cbuf_initialize(&key_buf, 32);

	i8042_flush();

	if (i8042_command(&ctr, I8042_CMD_CTL_RCTR)) {
		dprintf(SPEW, "Failed to read CTR while initializing i8042\n");
		return;
	}

	// turn on translation
	ctr |= I8042_CTR_XLATE;

	// enable keyboard and keyboard irq
	ctr &= ~I8042_CTR_KBDDIS;
	ctr |= I8042_CTR_KBDINT;

	if (i8042_command(&ctr, I8042_CMD_CTL_WCTR)) {
		dprintf(SPEW, "Failed to write CTR while initializing i8042\n");
		return;
	}

	register_int_handler(INT_KEYBOARD, &i8042_interrupt, NULL);
	unmask_interrupt(INT_KEYBOARD);

	i8042_interrupt(NULL);
}
