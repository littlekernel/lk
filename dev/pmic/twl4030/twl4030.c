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
#include <sys/types.h>
#include <dev/i2c.h>
#include <dev/twl4030.h>
#include "twl4030_hw.h"

// XXX move to target specific setup
#define TWL_I2C_BUS 0

void twl4030_init(void)
{
}

static int twl4030_usb_write(uint8_t address, uint8_t data)
{
	return i2c_write_reg(TWL_I2C_BUS, TWL_USB_ADDR, address, data);	
}

static int twl4030_usb_read(uint8_t address)
{
	uint8_t data;

	int err = i2c_read_reg(TWL_I2C_BUS, TWL_USB_ADDR, address, &data);	
	if (err < 0)
		return err;

	return data;
}

static int twl4030_usb_set_bits(uint8_t reg, uint8_t bits)
{
    return twl4030_usb_write(reg + 1, bits);
}

static int twl4030_usb_clear_bits(uint8_t reg, uint8_t bits)
{
    return twl4030_usb_write(reg + 2, bits);
}

static void twl4030_i2c_access(bool on)
{
	int val;

	if ((val = twl4030_usb_read(PHY_CLK_CTRL)) >= 0) {
		if (on) {
			/* enable DPLL to access PHY registers over I2C */
			val |= REQ_PHY_DPLL_CLK;
			twl4030_usb_write(PHY_CLK_CTRL, (uint8_t)val);

			while (!(twl4030_usb_read(PHY_CLK_CTRL_STS) & PHY_DPLL_CLK)) {
				spin(10);
			}
			if (!(twl4030_usb_read(PHY_CLK_CTRL_STS) & PHY_DPLL_CLK))
				printf("Timeout setting T2 HSUSB " "PHY DPLL clock\n");
		} else {
			/* let ULPI control the DPLL clock */
			val &= ~REQ_PHY_DPLL_CLK;
			twl4030_usb_write(PHY_CLK_CTRL, (uint8_t)val);
		}
	}
	return;
}

int twl4030_usb_reset(void)
{
	TRACE_ENTRY;
#if 0
    twl4030_usb_clear_bits(OTG_CTRL, DMPULLDOWN | DPPULLDOWN);
    twl4030_usb_clear_bits(USB_INT_EN_RISE, ~0);
    twl4030_usb_clear_bits(USB_INT_EN_FALL, ~0);
    twl4030_usb_clear_bits(MCPC_IO_CTRL, ~TXDTYP);
    twl4030_usb_set_bits(MCPC_IO_CTRL, TXDTYP);
    twl4030_usb_clear_bits(OTHER_FUNC_CTRL, (BDIS_ACON_EN | FIVEWIRE_MODE));
    twl4030_usb_clear_bits(OTHER_IFC_CTRL, ~0);
    twl4030_usb_clear_bits(OTHER_INT_EN_RISE, ~0);
    twl4030_usb_clear_bits(OTHER_INT_EN_FALL, ~0);
    twl4030_usb_clear_bits(OTHER_IFC_CTRL2, ~0);
    twl4030_usb_clear_bits(REG_CTRL_EN, ULPI_I2C_CONFLICT_INTEN);
    twl4030_usb_clear_bits(OTHER_FUNC_CTRL2, VBAT_TIMER_EN);
#endif

    /* Enable writing to power configuration registers */
    i2c_write_reg(TWL_I2C_BUS, TWL_PM_RECEIVER_ADDR, PROTECT_KEY, 0xC0);
    i2c_write_reg(TWL_I2C_BUS, TWL_PM_RECEIVER_ADDR, PROTECT_KEY, 0x0C);

    /* put VUSB3V1 LDO in active state */
    i2c_write_reg(TWL_I2C_BUS, TWL_PM_RECEIVER_ADDR, VUSB_DEDICATED2, 0);

    /* input to VUSB3V1 LDO is from VBAT, not VBUS */
    i2c_write_reg(TWL_I2C_BUS, TWL_PM_RECEIVER_ADDR, VUSB_DEDICATED1, 0x14);

    /* turn on 3.1V regulator */
    i2c_write_reg(TWL_I2C_BUS, TWL_PM_RECEIVER_ADDR, VUSB3V1_DEV_GRP, 0x20);
    i2c_write_reg(TWL_I2C_BUS, TWL_PM_RECEIVER_ADDR, VUSB3V1_TYPE, 0);

    /* turn on 1.5V regulator */
    i2c_write_reg(TWL_I2C_BUS, TWL_PM_RECEIVER_ADDR, VUSB1V5_DEV_GRP, 0x20);
    i2c_write_reg(TWL_I2C_BUS, TWL_PM_RECEIVER_ADDR, VUSB1V5_TYPE, 0);

    /* turn on 1.8V regulator */
    i2c_write_reg(TWL_I2C_BUS, TWL_PM_RECEIVER_ADDR, VUSB1V8_DEV_GRP, 0x20);
    i2c_write_reg(TWL_I2C_BUS, TWL_PM_RECEIVER_ADDR, VUSB1V8_TYPE, 0);

    /* disable access to power configuration registers */
    i2c_write_reg(TWL_I2C_BUS, TWL_PM_RECEIVER_ADDR, PROTECT_KEY, 0);

	/* turn on the phy */
    uint8_t pwr = twl4030_usb_read(PHY_PWR_CTRL);
	pwr &= ~PHYPWD;
	twl4030_usb_write(PHY_PWR_CTRL, pwr);
	twl4030_usb_write(PHY_CLK_CTRL,
		twl4030_usb_read(PHY_CLK_CTRL) |
		(CLOCKGATING_EN | CLK32K_EN));

	/* set DPLL i2c access mode */
	twl4030_i2c_access(true);
	/* set ulpi mode */
	twl4030_usb_clear_bits(IFC_CTRL, CARKITMODE);
	twl4030_usb_set_bits(POWER_CTRL, OTG_ENAB);
	twl4030_usb_write(FUNC_CTRL, XCVRSELECT_HS); // set high speed mode
//	twl4030_usb_write(FUNC_CTRL, XCVRSELECT_FS); // set full speed mode
	twl4030_i2c_access(false);

	return 0;
}

int twl4030_init_hs(void)
{
	return 0;
}

int twl4030_set_usb_pullup(bool pullup)
{
	TRACE_ENTRY;

	if (pullup) {
		twl4030_usb_clear_bits(OTG_CTRL, DPPULLDOWN);
		twl4030_usb_set_bits(FUNC_CTRL, TERMSELECT);
	} else {
		twl4030_usb_clear_bits(FUNC_CTRL, TERMSELECT);
		twl4030_usb_set_bits(OTG_CTRL, DPPULLDOWN);
	}

	return 0;
}


