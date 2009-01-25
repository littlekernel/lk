/*
 * Copyright (c) 2008 Travis Geiselbrecht
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
#include <debug.h>
#include <err.h>
#include <reg.h>
#include <string.h>
#include <platform.h>
#include <platform/omap3.h>

#define LOCAL_TRACE 0

#define I2C_TIMEOUT 200

static const addr_t i2c_reg_base[] = {
	I2C1_BASE,
	I2C2_BASE,
	I2C3_BASE,
};

#define I2C_REG_ADDR(bus, reg) (i2c_reg_base[bus] + (reg))
#define I2C_REG(bus, reg) (*REG16(I2C_REG_ADDR(bus, reg)))
#define I2C_RMW_REG(bus, reg, startbit, width, val) RMWREG16(I2C_REG_ADDR(bus, reg), startbit, width, val)

static void i2c_dump_bus(int bus)
{
	hexdump((void *)i2c_reg_base[bus], 128);
}

static void i2c_reset_bus(int bus)
{
	I2C_REG(bus, I2C_CON) &= ~(1<<15); // make sure the bus is disabled

	/* reset the bus */
	I2C_REG(bus, I2C_SYSC) = (1<<1);
	I2C_REG(bus, I2C_CON) = (1<<15); // enable the bus
	while ((I2C_REG(bus, I2C_SYSS) & 1) == 0)
		;

	/* disable the bus again and set up some internals */
	I2C_REG(bus, I2C_CON) &= ~(1<<15); // make sure the bus is disabled

	/* set up the clock */
	I2C_REG(bus, I2C_PSC) = 23; // 96Mhz / 23 == 4Mhz
	I2C_REG(bus, I2C_SCLL) = 13; 
	I2C_REG(bus, I2C_SCLH) = 15; // 4Mhz / combined divider of 40 (13+7 + 15+5) == 100khz

	/* slave address */
	I2C_REG(bus, I2C_OA0) = 1; // XXX made this up

	/* fifo is set to 1 byte trigger */
	I2C_REG(bus, I2C_BUF) = 0;

	/* disable all interrupts */
	I2C_REG(bus, I2C_IE) = 0;

	/* enable the bus */
	I2C_REG(bus, I2C_CON) = (1<<15)|(1<<10)|(1<<9); // enable, master, transmitter mode
}

static void i2c_wait_for_bb(int bus)
{
	I2C_REG(bus, I2C_STAT) = 0xffff; // clear whatever is pending
	while (I2C_REG(bus, I2C_STAT) & (1<<12)) {
		I2C_REG(bus, I2C_STAT) = 0xffff; // clear whatever is pending
	}
	I2C_REG(bus, I2C_STAT) = 0xffff; // clear whatever is pending
}

int i2c_transmit(int bus, uint8_t address, const void *buf, size_t count)
{
	int err;

	LTRACEF("bus %d, address 0x%hhx, buf %p, count %zd\n", bus, address, buf, count);
		
	i2c_wait_for_bb(bus);

	I2C_REG(bus, I2C_SA) = address;
	I2C_REG(bus, I2C_CNT) = count;
	I2C_REG(bus, I2C_CON) = (1<<15)|(1<<10)|(1<<9)|(1<<1)|(1<<0); // enable, master, transmit, STP, STT

	time_t t = current_time();

	const uint8_t *ptr = (const uint8_t *)buf;
	for(;;) {
		uint16_t stat = I2C_REG(bus, I2C_STAT);
		if (stat & (1<<1)) {
			// NACK
//			printf("NACK\n");
			err = -1;
			goto out;
		}
		if (stat & (1<<0)) {
			// AL (arbitration lost)
//			printf("arbitration lost!\n");
			err = -1;
			goto out;
		}
		if (stat & (1<<2)) {
			// ARDY
//			printf("ARDY, completed\n");
			break;
		}
		if (stat & (1<<4)) {
			// RRDY
//			printf("XRDY\n");

			// transmit a byte
			*REG8(I2C_REG_ADDR(bus, I2C_DATA)) = *ptr;
			ptr++;
		}
		I2C_REG(bus, I2C_STAT) = stat;

		if (current_time() - t > I2C_TIMEOUT) {
//			printf("i2c timeout\n");
			err = ERR_TIMED_OUT;
			goto out;
		}
	}

	err = 0;

out:
	I2C_REG(bus, I2C_STAT) = 0xffff;
	I2C_REG(bus, I2C_CNT) = 0;

	return err;
}

int i2c_receive(int bus, uint8_t address, void *buf, size_t count)
{
	int err;

	LTRACEF("bus %d, address 0x%hhx, buf %p, count %zd\n", bus, address, buf, count);
		
	i2c_wait_for_bb(bus);

	I2C_REG(bus, I2C_SA) = address;
	I2C_REG(bus, I2C_CNT) = count;
	I2C_REG(bus, I2C_CON) = (1<<15)|(1<<10)|(1<<1)|(1<<0); // enable, master, STP, STT

	time_t t = current_time();

	uint8_t *ptr = (uint8_t *)buf;
	for(;;) {
		uint16_t stat = I2C_REG(bus, I2C_STAT);
		if (stat & (1<<1)) {
			// NACK
//			printf("NACK\n");
			err = -1;
			goto out;
		}
		if (stat & (1<<0)) {
			// AL (arbitration lost)
//			printf("arbitration lost!\n");
			err = -1;
			goto out;
		}
		if (stat & (1<<2)) {
			// ARDY
//			printf("ARDY, completed\n");
			break;
		}
		if (stat & (1<<3)) {
			// RRDY
//			printf("RRDY\n");

			// read a byte, since our fifo threshold is set to 1 byte
			*ptr = *REG8(I2C_REG_ADDR(bus, I2C_DATA));
			ptr++;
		}
		I2C_REG(bus, I2C_STAT) = stat;

		if (current_time() - t > I2C_TIMEOUT) {
//			printf("i2c timeout\n");
			err = ERR_TIMED_OUT;
			goto out;
		}
	}

	err = 0;

out:
	I2C_REG(bus, I2C_STAT) = 0xffff;
	I2C_REG(bus, I2C_CNT) = 0;

	return err;
}

int i2c_write_reg(int bus, uint8_t address, uint8_t reg, uint8_t val)
{
	uint8_t buf[2];

	buf[0] = reg;
	buf[1] = val;

	return i2c_transmit(bus, address, buf, 2);
}

int i2c_read_reg(int bus, uint8_t address, uint8_t reg, uint8_t *val)
{
	int err = i2c_transmit(bus, address, &reg, 1);
	if (err < 0)
		return err;

	return i2c_receive(bus, address, val, 1);
}


void i2c_init_early(void)
{
	LTRACE_ENTRY;

	/* enable clocks on i2c 0-2 */
	RMWREG32(CM_FCLKEN1_CORE, 15, 3, 0x7),
	RMWREG32(CM_ICLKEN1_CORE, 15, 3, 0x7),

	i2c_reset_bus(0);
	i2c_reset_bus(1);
	i2c_reset_bus(2);

#if 0
	// write something into a reg
	char buf[2];
	i2c_write_reg(0, 0x4b, 0x14, 0x99);
	i2c_write_reg(0, 0x4b, 0x15, 0x98);

	i2c_read_reg(0, 0x4b, 0x15, buf);
	printf("0x%hhx\n", buf[0]);
	i2c_read_reg(0, 0x4b, 0x14, buf);
	printf("0x%hhx\n", buf[0]);

	int i;
	for (i=0; i < 255; i++) {
		char buf[1];
		buf[0] = i;
		i2c_transmit(0, 0x4b, buf, 1);
		i2c_receive(0, 0x4b, buf, sizeof(buf));
		printf("0x%hhx\n", buf[0]);
	}
#endif

	LTRACE_EXIT;
}

void i2c_init(void)
{
}

#if WITH_LIB_CONSOLE

#include <lib/console.h>

static int cmd_i2c(int argc, const cmd_args *argv);

STATIC_COMMAND_START
	{ "i2c", "i2c read/write commands", &cmd_i2c },
STATIC_COMMAND_END(i2c);

static int cmd_i2c(int argc, const cmd_args *argv)
{
	int err;

	if (argc < 5) {
		printf("not enough arguments\n");
usage:
		printf("%s read_reg <bus> <i2c address> <register>\n", argv[0].str);
		printf("%s write_reg <bus> <i2c address> <register> <val>\n", argv[0].str);
		return -1;
	}

	int bus = argv[2].u;
	uint8_t i2c_address = argv[3].u;

	if (!strcmp(argv[1].str, "read_reg")) {
		uint8_t reg = argv[4].u;
		uint8_t val;

		err = i2c_read_reg(bus, i2c_address, reg, &val);
		printf("i2c_read_reg err %d, val 0x%hhx\n", err, val);
	} else if (!strcmp(argv[1].str, "write_reg")) {
		uint8_t reg = argv[4].u;
		uint8_t val = argv[5].u;
		err = i2c_write_reg(bus, i2c_address, reg, val);
		printf("i2c_write_reg err %d\n", err);
	} else {
		printf("unrecognized subcommand\n");
		goto usage;
	}

	return 0;
}

#endif // WITH_APP_CONSOLE


