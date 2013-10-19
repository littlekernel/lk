/*
 * Copyright (c) 2013 Travis Geiselbrecht
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
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <trace.h>
#include <platform.h>
#include <reg.h>
#include <dev/usb.h>
#include <dev/usbc.h>
#include <arch/arm/cm.h>

#include "ti_driverlib.h"
#include "inc/hw_usb.h"

#define LOCAL_TRACE 0

static bool pending_addr_change = false;
static uint8_t addr;

static void usbc_dump_regs(void)
{
	printf("USB0 reg dump:\n");
#define DUMPREG8(r) printf("\t" #r ": 0x%hhx\n", *REG8(USB0_BASE + (r)));
#define DUMPREG16(r) printf("\t" #r ": 0x%hx\n", *REG16(USB0_BASE + (r)));
#define DUMPREG32(r) printf("\t" #r ": 0x%x\n",  *REG32(USB0_BASE + (r)));

	DUMPREG8(USB_O_FADDR);
	DUMPREG8(USB_O_POWER);
	DUMPREG16(USB_O_TXIS);
	DUMPREG16(USB_O_RXIS);
	DUMPREG16(USB_O_TXIE);
	DUMPREG16(USB_O_RXIE);
	DUMPREG8(USB_O_IS);
	DUMPREG8(USB_O_IE);
	DUMPREG16(USB_O_FRAME);
	DUMPREG8(USB_O_EPIDX);
	DUMPREG8(USB_O_TEST);
	DUMPREG32(USB_O_FIFO0);
	DUMPREG32(USB_O_FIFO1);
	DUMPREG32(USB_O_FIFO2);
	DUMPREG32(USB_O_FIFO3);
	DUMPREG32(USB_O_FIFO4);
	DUMPREG32(USB_O_FIFO5);
	DUMPREG32(USB_O_FIFO6);
	DUMPREG32(USB_O_FIFO7);
	DUMPREG32(USB_O_FIFO8);
	DUMPREG32(USB_O_FIFO9);
	DUMPREG32(USB_O_FIFO10);
	DUMPREG32(USB_O_FIFO11);
	DUMPREG32(USB_O_FIFO12);
	DUMPREG32(USB_O_FIFO13);
	DUMPREG32(USB_O_FIFO14);
	DUMPREG32(USB_O_FIFO15);
	DUMPREG16(USB_O_DEVCTL);
	DUMPREG8(USB_O_TXFIFOSZ);
	DUMPREG8(USB_O_RXFIFOSZ);
	DUMPREG16(USB_O_TXFIFOADD);
	DUMPREG16(USB_O_RXFIFOADD);
	DUMPREG32(USB_O_PP);

#undef DUMPREG8
#undef DUMPREG16
#undef DUMPREG32
}

void stellaris_usbc_early_init(void)
{
	LTRACE_ENTRY;
	LTRACE_EXIT;
}

void stellaris_usbc_init(void)
{
	LTRACE_ENTRY;

	SysCtlPeripheralEnable(SYSCTL_PERIPH_USB0);
	SysCtlPeripheralReset(SYSCTL_PERIPH_USB0);

	SysCtlUSBPLLEnable();

	GPIOPinTypeUSBAnalog(GPIO_PORTD_AHB_BASE, GPIO_PIN_4 | GPIO_PIN_5);

	USBDevMode(USB0_BASE);
	USBPHYPowerOn(USB0_BASE);

#if LOCAL_TRACE
	usbc_dump_regs();

	printf("addr %lu\n", USBDevAddrGet(USB0_BASE));
	printf("ep0 status 0x%lx\n", USBEndpointStatus(USB0_BASE, USB_EP_0));
#endif

	NVIC_EnableIRQ(INT_USB0 - 16);
	USBIntDisableControl(USB0_BASE, USB_INTCTRL_ALL);

	LTRACE_EXIT;
}

static void ep0_irq(void)
{
	uint status = USBEndpointStatus(USB0_BASE, USB_EP_0);

	LTRACEF("ep0 status 0x%x\n", status);

	/* delay setting the address until the ack as completed */
	if (pending_addr_change) {
		LTRACEF("pending addr change\n");
		USBDevAddrSet(USB0_BASE, addr);
		pending_addr_change = false;
	}

	if (status & USB_DEV_EP0_OUT_PKTRDY) {
		LTRACEF("pktrdy\n");

		uchar buf[sizeof(struct usb_setup)];
		ulong avail = sizeof(buf);

		if (USBEndpointDataGet(USB0_BASE, USB_EP_0, buf, &avail) < 0 || avail != sizeof(buf)) {
			LTRACEF("short setup packet, size %lu\n", avail);
		} else {
			union usb_callback_args args;
			args.setup = (void *)buf;
			usb_callback(USB_CB_SETUP_MSG, &args);
		}
	}
	if (status & USB_DEV_EP0_SENT_STALL) {
		LTRACEF("stall complete\n");
		USBDevEndpointStallClear(USB0_BASE, USB_EP_0, 0);
	}
}

void stellaris_usb0_irq(void)
{
	arm_cm_irq_entry();

	uint status = USBIntStatusControl(USB0_BASE);

	//LTRACEF("usb irq, status 0x%x\n", status);

	if (status & USB_INTCTRL_RESET) {
		// reset
		LTRACEF("reset\n");
		pending_addr_change = false;
		usb_callback(USB_CB_RESET, NULL);
	}
	if (status & USB_INTCTRL_CONNECT) {
		// reset
		LTRACEF("connect\n");
	}

	status = USBIntStatusEndpoint(USB0_BASE);

	if (status & USB_INTEP_0) {
		// ep0
		//LTRACEF("ep0\n");
		ep0_irq();
	}

	arm_cm_irq_exit(true);
}

void usbc_ep0_ack(void)
{
	LTRACE_ENTRY;

	USBDevEndpointDataAck(USB0_BASE, USB_EP_0, true);
}

void usbc_ep0_stall(void)
{
	LTRACE_ENTRY;

	USBDevEndpointStall(USB0_BASE, USB_EP_0, 0);
}

void usbc_ep0_send(const void *buf, size_t len, size_t maxlen)
{
	LTRACEF("buf %p, len %zu, maxlen %zu\n", buf, len, maxlen);

	USBEndpointDataPut(USB0_BASE, USB_EP_0, (void *)buf, MIN(len, maxlen));

	USBEndpointDataSend(USB0_BASE, USB_EP_0, USB_TRANS_SETUP);
}

void usbc_set_address(uint8_t address)
{
	LTRACEF("address 0x%hhx\n", address);

	addr = address;
	pending_addr_change = true;
}

void usbc_ep0_recv(void *buf, size_t len, ep_callback cb)
{
	PANIC_UNIMPLEMENTED;
}

bool usbc_is_highspeed(void)
{
	return false;
}

status_t usbc_set_active(bool active)
{
	LTRACEF("active %d\n", active);
	if (active) {
		USBIntEnableControl(USB0_BASE, USB_INTCTRL_CONNECT | USB_INTCTRL_RESET);
		USBIntEnableEndpoint(USB0_BASE, USB_INTEP_0);
		USBDevConnect(USB0_BASE);
	} else {
		USBDevDisconnect(USB0_BASE);
	}

	return NO_ERROR;
}

status_t usbc_setup_endpoint(ep_t ep, ep_dir_t dir, uint width)
{
	PANIC_UNIMPLEMENTED;
}

status_t usbc_queue_rx(ep_t ep, usbc_transfer_t *transfer)
{
	PANIC_UNIMPLEMENTED;
}

status_t usbc_queue_tx(ep_t ep, usbc_transfer_t *transfer)
{
	PANIC_UNIMPLEMENTED;
}

// vim: set ts=4 sw=4 noexpandtab:
