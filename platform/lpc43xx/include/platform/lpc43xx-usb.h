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

#define USB0_BASE		0x40006000

#define USB_CMD			0x140
#define USB_STS			0x144
#define USB_INTR		0x148
#define USB_FRINDEX		0x14C
#define USB_DEVICEADDR		0x154
#define USB_ENDPOINTLISTADDR 	0x158
#define USB_BURSTSIZE		0x160
#define USB_ENDPTNAK		0x178
#define USB_ENDPTNAKEN		0x17C
#define USB_PORTSC1		0x184
#define USB_OTGSC		0x1A4
#define USB_MODE		0x1A8
#define USB_ENDPTSETUPSTAT	0x1AC
#define USB_ENDPTPRIME		0x1B0
#define USB_ENDPTFLUSH		0x1B4
#define USB_ENDPTSTAT		0x1B8
#define USB_ENDPTCOMPLETE	0x1BC
#define USB_ENDPTCTRL(n)	(0x1C0 + (n) * 4)


#define CMD_RUN			(1 << 0) // initiate attach
#define CMD_STOP		(0 << 0) // detach
#define CMD_RST			(1 << 1) // reset controller, raz when done
#define CMD_SUTW		(1 << 13) // SetUp TripWire
#define CMD_ATDTW		(1 << 14) // Add TD TripWire
#define CMD_ITC_0		(0 << 16) // IRQ Threshold Immediate
#define CMD_ITC_1		(1 << 16) // 1 microframe
#define CMD_ITC_2		(2 << 16) // 2 microframes
#define CMD_ITC_8		(8 << 16) // 8 microframes
#define CMD_ITC_16		(16 << 16) // 16 microframes
#define CMD_ITC_32		(32 << 16) // 32 microframes
#define CMD_ITC_64		(64 << 16) // 64 microframes

#define STS_UI			(1 << 0) // USB Interrupt (WtC)
#define STS_UEI			(1 << 1) // USB Error IRQ (WtC)
#define STS_PCI			(1 << 2) // Port Change Detect (WtC)
#define STS_SEI			(1 << 4) // System Error (fatal, reset)
#define STS_URI			(1 << 6) // USB Reset (WtC)
#define STS_SRI			(1 << 7) // SOF Received (WtC)
#define STS_SLI			(1 << 8) // DC Suspend (WtC)
#define STS_NAKI		(1 << 16) // 1 when EPT NAK bits set

#define INTR_UE			(1 << 0) // USB Interrupt Enable
#define INTR_UEE		(1 << 1) // USB Error IRQ Enable
#define INTR_PCE		(1 << 2) // Port Change Detect IRQ Enable
#define INTR_SEE		(1 << 4) // System Error IRQ Enable
#define INTR_URE		(1 << 6) // USB Reset IRQ Enable
#define INTR_SRE		(1 << 7) // SOF Received IRQ Enable
#define INTR_SLE		(1 << 8) // DC Suspend IRQ Enable
#define INTR_NAKE		(1 << 16) // NAK IRQ Enable

#define PORTSC1_CCS		(1 << 0) // device is attached
#define PORTSC1_PE		(1 << 2) // port enable (always 1)
#define PORTSC1_PEC		(1 << 3) // always 0
#define PORTSC1_FPR		(1 << 6) // force port resume
#define PORTSC1_SUSP		(1 << 7) // ro: 1 = port suspended
#define PORTSC1_RC		(1 << 8) // ro: 1 = port in reset
#define PORTSC1_HSP		(1 << 9) // ro: 1 = port in high-speed
#define PORTSC1_PFSC		(1 << 24) // 1 = force full-speed only

#define OTG_VD			(1 << 0)  // vbus discharge
#define OTG_VC			(1 << 1)  // vbus charge
#define OTG_HAAR		(1 << 2)  // hardware assist auto-reset
#define OTG_OT			(1 << 3)  // OTG termination
#define OTG_DP			(1 << 4)  // data pulsing
#define OTG_IDPU		(1 << 5)  // ID pull-up
#define OTG_HADP		(1 << 6)  // hardware assist data pulse
#define OTG_HABA		(1 << 7)  // hardware assist B-dis to A-con
#define OTG_ID			(1 << 8)  // 0 = A-device, 1 = B-device
#define OTG_AVV			(1 << 9)  // A-VBUS Valid
#define OTG_ASV			(1 << 10) // A-Session Valid
#define OTG_BSV			(1 << 11) // B-Session Valid
#define OTG_BSE			(1 << 12) // B-Session End
#define OTG_MS1T		(1 << 13) // 1ms Timer Toggle
#define OTG_DPS			(1 << 14) // 1 = data pulsing detected
#define OTG_IDIS		(1 << 16) // irq status bits (r/wc)
#define OTG_AVVIS		(1 << 17)
#define OTG_ASVIS		(1 << 18)
#define OTG_BSVIS		(1 << 19)
#define OTG_BSEIS		(1 << 20)
#define OTG_MS1S		(1 << 21)
#define OTG_DPIS		(1 << 22)
#define OTG_IDIE		(1 << 24) // irq enable bits (rw)
#define OTG_AVVIE		(1 << 25)
#define OTG_ASVIE		(1 << 26)
#define OTG_BSVIE		(1 << 27)
#define OTG_BSEIE		(1 << 28)
#define OTG_MS1E		(1 << 29)
#define OTG_DPIE		(1 << 30)

//#define MODE_MASK		3
#define MODE_IDLE		0 // write once to enter device/host mode
#define MODE_DEVICE		2 // must reset to idle to change mode
#define MODE_HOST		3 // nust reset to idle to change mode
#define MODE_ES			(1 << 2) // select big endian
#define MODE_SLOM		(1 << 3) // enable setup lockout mode
#define MODE_SDIS		(1 << 4) // enable stream disable mode

// bits for ENDPTCTRL(n)
#define EPCTRL_RXS		(1 << 0) // rx ept stall
#define EPCTRL_RX_CTRL		(0 << 2)
#define EPCTRL_RX_ISOC		(1 << 2)
#define EPCTRL_RX_BULK		(2 << 2)
#define EPCTRL_RX_INTR		(3 << 2)
#define EPCTRL_RXR		(1 << 6) // tx data toggle reset
#define EPCTRL_RXE		(1 << 7) // rx ept enable
#define EPCTRL_TXS		(1 << 16) // tx ept stall
#define EPCTRL_TX_CTRL		(0 << 18)
#define EPCTRL_TX_ISOC		(1 << 18)
#define EPCTRL_TX_BULK		(2 << 18)
#define EPCTRL_TX_INTR		(3 << 18)
#define EPCTRL_TXR		(1 << 22) // tx data toggle reset
#define EPCTRL_TXE		(1 << 23) // rx ept enable
// Do not leave unconfigured endpoints as CTRL when enabling
// their sibling, or data PID tracking will be undefined

// bits for all other ENDPT* registers
#define EP0_RX			(1 << 0)
#define EP1_RX			(1 << 1)
#define EP2_RX			(1 << 2)
#define EP3_RX			(1 << 3)
#define EP4_RX			(1 << 4)
#define EP5_RX			(1 << 5)
#define EP0_TX			(1 << 16)
#define EP1_TX			(1 << 17)
#define EP2_TX			(1 << 18)
#define EP3_TX			(1 << 19)
#define EP4_TX			(1 << 20)
#define EP5_TX			(1 << 21)

#define EPT_TX(n) (1 << ((n) + 16))
#define EPT_RX(n) (1 << (n))

#define DQH_MULT0		(0 << 30) // non-iscoh
#define DQH_MULT1		(1 << 30) // 1 txn per td
#define DQH_MULT2		(2 << 30) // 2 txn per td
#define DQH_MULT3		(3 << 30) // 3 txn per td
#define DQH_CFG_ZLT		(1 << 29) // disable zero-length terminate
#define DQH_CFG_MAXPKT(n)	((n) << 16) // <= 1024
#define DQH_CFG_IOS		(1 << 15) // IRQ on SETUP

#define DTD_LEN(n)		((n) << 16)
#define DTD_IOC			(1 << 15) // interrupt on complete
#define DTD_MULT0		(0 << 10)
#define DTD_MULT1		(1 << 10)
#define DTD_MULT2		(2 << 10)
#define DTD_MULT3		(3 << 10)
#define DTD_STS_MASK		0xE8
#define DTD_ACTIVE		0x80
#define DTD_HALTED		0x40
#define DTD_BUF_ERR		0x20
#define DTD_TXN_ERR		0x08

typedef struct usb_dtd {
	u32 next_dtd;
	u32 config;
	u32 bptr0;
	u32 bptr1;
	u32 bptr2;
	u32 bptr3;
	u32 bptr4;
	struct usb_dtd *next;
} usb_dtd_t;

typedef struct usb_dqh {
	u32 config;
	u32 current_dtd;
	u32 next_dtd;
	u32 dtd_config;
	u32 dtd_bptr0;
	u32 dtd_bptr1;
	u32 dtd_bptr2;
	u32 dtd_bptr3;
	u32 dtd_bptr4;
	u32 dtd_rsvd0;
	u32 setup0;
	u32 setup1;
	u32 rsvd1;
	u32 rsvd2;
	u32 rsvd3;
	u32 rsvd4;
} usb_dqh_t;
