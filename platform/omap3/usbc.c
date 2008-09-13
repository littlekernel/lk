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
#include <reg.h>
#include <dev/usb.h>
#include <dev/usbc.h>
#include <dev/twl4030.h>
#include <hw/usb.h>
#include <platform/interrupts.h>
#include <platform/omap3.h>

#define LOCAL_TRACE 1

#define hsusb_reg8(reg) *REG8(USB_HS_BASE + (reg))
#define hsusb_reg16(reg) *REG16(USB_HS_BASE + (reg))
#define hsusb_reg32(reg) *REG32(USB_HS_BASE + (reg))

/* registers */
#define FADDR	0x0
#define POWER	0x1
#define INTRTX	0x2
#define INTRRX	0x4
#define INTRTXE	0x6
#define INTRRXE	0x8
#define INTRUSB	0xa
#define INTRUSBE 0xb
#define FRAME 	0xc
#define INDEX 	0xe
#define TESTMODE 0xf

// indexed endpoint regs
#define IDX_TXMAXP	0x10
#define IDX_TXCSR	0x12
#define IDX_TXCSRL	0x12
#define IDX_TXCSRH	0x13
#define IDX_RXMAXP	0x14
#define IDX_RXCSR	0x16
#define IDX_RXCSRL	0x16
#define IDX_RXCSRH	0x17
#define IDX_RXCOUNT	0x18
#define IDX_FIFOSIZE	0x1f

// if endpoint 0 is selected
#define IDX_CSR0	0x12
#define IDX_CONFIGDATA	0x1f

// endpoint FIFOs
#define FIFOBASE	0x20

#define DEVCTL	0x60
#define TXFIFOSZ	0x62
#define RXFIFOSZ	0x63
#define TXFIFOADD	0x64
#define RXFIFOADD	0x66
#define HWVERS	0x6c
#define EPINFO	0x78
#define RAMINFO	0x79
#define LINKINFO	0x7a

static enum handler_return hsusb_interrupt(void *arg)
{
	uint16_t intrtx = hsusb_reg16(INTRTX);
	uint16_t intrrx = hsusb_reg16(INTRRX);
	uint8_t intrusb = hsusb_reg8(INTRUSB);
	enum handler_return ret = INT_NO_RESCHEDULE;

	LTRACEF("intrtx 0x%hx (0x%x), intrrx 0x%hx (0x%x), intrusb 0x%hhx, intrusbe 0x%hhx\n", 
			intrtx, hsusb_reg16(INTRTXE), intrrx, hsusb_reg16(INTRRXE), intrusb, hsusb_reg8(INTRUSBE));

	return ret;
}

int usbc_set_active(bool active)
{
	if (active) {
//		DEBUG_ASSERT(!usbc->active);

		hsusb_reg8(POWER) |= (1<<6); // soft conn
		twl4030_set_usb_pullup(true);
//		usbc->active = true;
	} else {
		hsusb_reg8(POWER) &= ~(1<<6); // soft conn
		twl4030_set_usb_pullup(false);
//		usbc->active = false;
	}

	return 0;
}

static void otg_reset(void)
{
	/* reset the chip */
	*REG32(OTG_SYSCONFIG) |= (1<<1);
	while ((*REG32(OTG_SYSSTATUS) & 1) == 0)
		;

	/* power up the controller */
	*REG32(OTG_FORCESTDBY) = 0; // disable forced standby
	*REG32(OTG_SYSCONFIG) &= ~(1<<1); // autoidle off
	*REG32(OTG_SYSCONFIG) = (2<<12) | (2<<3) | (0<<0); // master in smart-standby, periph in smart-idle, autoidle off

	*REG32(OTG_SYSCONFIG) |= 1; // autoidle on

	*REG32(OTG_INTERFSEL) = 1; // 8 bit ULPI
}

static void hsusb_init(void)
{
	LTRACE_ENTRY;

	// select endpoint 0
	dprintf(INFO, "hwvers 0x%hx\n", hsusb_reg16(HWVERS)); 
	dprintf(INFO, "epinfo 0x%hhx\n", hsusb_reg8(EPINFO)); 
	dprintf(INFO, "raminfo 0x%hhx\n", hsusb_reg8(RAMINFO)); 
	hsusb_reg8(INDEX) = 0;
	dprintf(INFO, "config 0x%hhx\n", hsusb_reg8(IDX_CONFIGDATA));

	// assert that we have dynamic fifo sizing
	DEBUG_ASSERT(hsusb_reg8(IDX_CONFIGDATA) & (1<<2));

	// mask all the interrupts for the endpoints (except 0)
	hsusb_reg16(INTRTXE) = (1<<0);
	hsusb_reg16(INTRRXE) = 0;

	twl4030_usb_reset();
	twl4030_init_hs();

	hsusb_reg8(DEVCTL) = 0; // peripheral mode
//	hsusb_reg8(POWER) &= (1<<5); // disable high speed
	hsusb_reg8(POWER) |= (1<<5); // enable high speed

	hsusb_reg8(INTRUSBE) = (1<<5)|(1<<2)|(1<<1)|(1<<0); // disconnect, reset, resume, suspend

	LTRACE_EXIT;
}

void usbc_init(void)
{
	// enable the clock
	RMWREG32(CM_ICLKEN1_CORE, 4, 1, 1);

	// register the interrupt handlers
	register_int_handler(92, hsusb_interrupt, NULL);
//	register_int_handler(93, hsusb_dma_interrupt, NULL);

	otg_reset();
	hsusb_init();

	unmask_interrupt(92);
//	unmask_interrupt(93);

	usbc_set_active(true);
}

