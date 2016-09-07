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

#include <app.h>
#include <debug.h>
#include <reg.h>
#include <kernel/thread.h>
#include <kernel/event.h>
#include <arch/arm/cm.h>

#include <driverlib/prcm.h>
#include <driverlib/rfc.h>
#include <driverlib/rf_mailbox.h>
#include <inc/hw_rfc_dbell.h>

#include <rf_patches/rf_patch_cpe_genfsk.h>

#include <platform/radio.h>

#define RADIO_POLLED_MODE 0

#define CPE0_MASK (IRQ_BOOT_DONE | IRQ_RX_OK | IRQ_LAST_COMMAND_DONE | IRQ_COMMAND_DONE)

static event_t ack_evt = EVENT_INITIAL_VALUE(ack_evt, 0, EVENT_FLAG_AUTOUNSIGNAL);

static event_t cpe0_evt = EVENT_INITIAL_VALUE(cpe0_evt, 0, EVENT_FLAG_AUTOUNSIGNAL);

void ti_cc_rfc_cpe_0_irq(void) {
	arm_cm_irq_entry();
	event_signal(&cpe0_evt, false);

	// disable IRQ until thread handles and re-enables them in response to event
	NVIC_DisableIRQ(rfc_cpe_0_IRQn);

	// reschedule if we woke a thread (indicated by !signaled)
	arm_cm_irq_exit(!cpe0_evt.signaled);
}

static inline uint32_t cpe0_reason(void) {
	uint32_t n = HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFCPEIFG) & CPE0_MASK;
	HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFCPEIFG) = ~n;
	return n;
}

static inline uint32_t cpe0_wait_irq(void) {
	NVIC_EnableIRQ(rfc_cpe_0_IRQn);
	event_wait(&cpe0_evt);
	return cpe0_reason();
}

void ti_cc_rfc_cpe_1_irq(void) {
	arm_cm_irq_entry();
}

void ti_cc_rfc_cmd_ack_irq(void) {
	arm_cm_irq_entry();
	HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFACKIFG) = 0;
	event_signal(&ack_evt, false);
	// reschedule if we woke a thread (indicated by !signaled)
	arm_cm_irq_exit(!ack_evt.signaled);
}

uint32_t radio_send_cmd(uint32_t cmd) {
#if RADIO_POLLED_MODE
	while (HWREG(RFC_DBELL_BASE + RFC_DBELL_O_CMDR) != 0) {}
	HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFACKIFG) = 0;
	HWREG(RFC_DBELL_BASE + RFC_DBELL_O_CMDR) = cmd;
	while (!HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFACKIFG)) {}
	HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFACKIFG) = 0;
	return HWREG(RFC_DBELL_BASE + RFC_DBELL_O_CMDSTA);
#else
	while(HWREG(RFC_DBELL_BASE + RFC_DBELL_O_CMDR) != 0) {}
	HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFACKIFG) = 0;
	event_unsignal(&ack_evt);
	HWREG(RFC_DBELL_BASE + RFC_DBELL_O_CMDR) = cmd;
	event_wait(&ack_evt);
#endif
	return HWREG(RFC_DBELL_BASE + RFC_DBELL_O_CMDSTA);
}

void radio_wait_cmd(uint16_t *status) {
	uint32_t addr = (uint32_t) status;
	uint16_t val;
#if RADIO_POLLED_MODE
	for (;;) {
		val = *REG16(addr);
		if (val < 3) {
			// idle, waiting to start, or running
			thread_yield();
		} else {
			break;
		}
	}
#else
	for (;;) {
		uint32_t x = cpe0_wait_irq();
		val = *REG16(addr);
		if (val > 3) {
			break;
		}
	}
#endif
	if ((val != 0x0400) && (val != 0x3400)) {
		dprintf(INFO, "Cmd Status %04x\n", val);
	}
}

void radio_init(void) {
#if !RADIO_POLLED_MODE
	// route all IRQs to CPE0
	HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFCPEISL) = 0;
	HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFCPEIEN) = CPE0_MASK;

	// clear any pending
	HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFCPEIFG) = 0;
	HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFACKIFG) = 0;

	//NVIC_EnableIRQ(rfc_cpe_0_IRQn);
	//NVIC_EnableIRQ(rfc_cpe_1_IRQn);
	NVIC_EnableIRQ(rfc_cmd_ack_IRQn);
#endif

	// Power RF domain
	PRCMPowerDomainOn(PRCM_DOMAIN_RFCORE);
	while (PRCMPowerDomainStatus(PRCM_DOMAIN_RFCORE) != PRCM_DOMAIN_POWER_ON) ;
	dprintf(INFO, "power on\n");

	// enable the RF top clock
	PRCMDomainEnable(PRCM_DOMAIN_RFCORE);
	PRCMLoadSet();
	dprintf(INFO, "top clock on\n");

	// enable all RF sub clocks
	RFCClockEnable();
	dprintf(INFO, "clocks on\n");

	thread_sleep(1000);
	rf_patch_cpe_genfsk();
	dprintf(INFO, "patched\n");

	unsigned n = radio_send_cmd(IMM_CMD(CMD_PING, 0, 0));
	dprintf(INFO, "RESPONSE %08x\n", n);
	n = radio_send_cmd(IMM_CMD(CMD_PING, 0, 0));
	dprintf(INFO, "RESPONSE %08x\n", n);

	rf_op_fw_info_t fwinfo;
	memset(&fwinfo, 0, sizeof(fwinfo));
	fwinfo.cmd = CMD_GET_FW_INFO;
	n = radio_send_cmd((uint32_t) &fwinfo);
	dprintf(INFO, "FW %d %04x %04x %04x %04x\n",
		n, fwinfo.version, fwinfo.free_ram_start,
		fwinfo.free_ram_size, fwinfo.avail_rat_ch);

	n = radio_send_cmd(IMM_CMD(CMD_START_RAT, 0, 0));
	dprintf(INFO, "START RAT %d\n", n);
}
