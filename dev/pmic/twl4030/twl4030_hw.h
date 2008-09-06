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
#ifndef __TWL4030_HW_H
#define __TWL4030_HW_H

/* TWL i2c addresses */
#define TWL_I2C_ADDR0 0x48
#define TWL_I2C_ADDR1 0x49
#define TWL_I2C_ADDR2 0x4a
#define TWL_I2C_ADDR3 0x4b

#define TWL_USB_ADDR	TWL_I2C_ADDR0
#define TWL_INTBR_ADDR	TWL_I2C_ADDR1
#define TWL_PM_RECEIVER_ADDR	TWL_I2C_ADDR3

/* TWL registers */
#define PROTECT_KEY		0x44
#define VAUX1_DEV_GRP	0x72
#define VAUX1_TYPE		0x73
#define VAUX1_REMAP		0x74
#define VAUX1_DEDICATED	0x75
#define VAUX2_DEV_GRP	0x76
#define VAUX2_TYPE		0x77
#define VAUX2_REMAP		0x78
#define VAUX2_DEDICATED	0x79
#define VAUX3_DEV_GRP	0x7a
#define VAUX3_TYPE		0x7b
#define VAUX3_REMAP		0x7c
#define VAUX3_DEDICATED	0x7d
#define VAUX4_DEV_GRP	0x7e
#define VAUX4_TYPE		0x7f
#define VAUX4_REMAP		0x80
#define VAUX4_DEDICATED	0x81
#define VMMC1_DEV_GRP	0x82
#define VMMC1_TYPE		0x83
#define VMMC1_REMAP		0x84
#define VMMC1_DEDICATED	0x85
#define VMMC2_DEV_GRP	0x86
#define VMMC2_TYPE		0x87
#define VMMC2_REMAP		0x88
#define VMMC2_DEDICATED	0x89
#define VPLL1_DEV_GRP	0x8a
#define VPLL1_TYPE		0x8b
#define VPLL1_REMAP		0x8c
#define VPLL1_DEDICATED	0x8d
#define VPLL2_DEV_GRP	0x8e
#define VPLL2_TYPE		0x8f
#define VPLL2_REMAP		0x90
#define VPLL2_DEDICATED	0x91
#define VDAC_DEV_GRP		0x96
#define VDAC_DEDICATED		0x99
#define VDD2_DEV_GRP		0xBE
#define VDD2_TYPE		0xBF
#define VDD2_REMAP		0xC0
#define VDD2_CFG		0xC1
#define VSIM_DEV_GRP		0x92
#define VSIM_TYPE		0x93
#define VSIM_REMAP		0x94
#define VSIM_DEDICATED		0x95
#define PMBR1			0x92

#define SECONDS_REG		0x1C
#define MINUTES_REG		0x1D
#define ALARM_SECONDS_REG	0x23
#define ALARM_MINUTES_REG	0x24
#define ALARM_HOURS_REG		0x25

#define RTC_STATUS_REG		0x2A
#define RTC_INTERRUPTS_REG	0x2B

#define PWR_ISR1		0x2E
#define PWR_IMR1		0x2F
#define PWR_ISR2		0x30
#define PWR_IMR2		0x31
#define PWR_EDR1		0x33

#define CFG_PWRANA2		0x3F
#define RTC_INTERRUPTS_REG	0x2B
#define STS_HW_CONDITIONS	0x45

#define P1_SW_EVENTS		0x46
#define P2_SW_EVENTS		0x47
#define P3_SW_EVENTS		0x48

#define VDD1_TRIM1		0x62
#define VDD1_TRIM2		0x63
#define VDD1_VFLOOR		0xBB
#define VDD1_VROOF		0xBC
#define PB_CFG			0x4A
#define PB_WORD_MSB		0x4B
#define PB_WORD_LSB		0x4C

#define VSIM_REMAP		0x94
#define VDAC_REMAP		0x98
#define VINTANA1_DEV_GRP	0x9A
#define VINTANA1_REMAP		0x9C
#define VINTANA2_REMAP		0xA0
#define VINTANA2_DEV_GRP	0x9E
#define VINTDIG_DEV_GRP		0xA2
#define VINTDIG_REMAP		0xA4
#define VIO_DEV_GRP		0xA6
#define VIO_REMAP		0xA8
#define VDD1_REMAP		0xB2
#define VDD2_REMAP		0xC0
#define REGEN_REMAP		0xDC
#define NRESPWRON_REMAP		0xDF
#define CLKEN_REMAP		0xE2
#define SYSEN_REMAP		0xE5
#define HFCLKOUT_REMAP		0xE8
#define HFCLKOUT_DEV_GRP	0xE6
#define T32KCLKOUT_REMAP	0xEB
#define TRITON_RESET_REMAP	0xEE
#define MAINREF_REMAP		0xF1
#define VIBRA_CTL		0x45


#define VUSB1V5_DEV_GRP 0xCC
#define VUSB1V5_TYPE 	0xCD
#define VUSB1V5_REMAP 	0xCE
#define VUSB1V8_DEV_GRP 0xCF
#define VUSB1V8_TYPE 	0xD0
#define VUSB1V8_REMAP 	0xD1
#define VUSB3V1_DEV_GRP 0xD2
#define VUSB3V1_TYPE 	0xD3
#define VUSB3V1_REMAP 	0xD4
#define VUSBCP_DEV_GRP 	0xD5
#define VUSBCP_TYPE 	0xD6
#define VUSBCP_REMAP 	0xD7
#define VUSB_DEDICATED1 0xD8
#define VUSB_DEDICATED2 0xD9

/* USB registers */
#define VENDOR_ID_LO	0x0
#define VENDOR_ID_HI	0x1
#define PRODUCT_ID_LO	0x2
#define PRODUCT_ID_HI	0x3
#define FUNC_CTRL		0x4
#define FUNC_CTRL_SET	0x5
#define FUNC_CTRL_CLR	0x6
#   define  SUSPENDM        (1 << 6)
#   define  RESET           (1 << 5)
#   define  OPMODE_MASK     (3 << 3) /* bits 3 and 4 */
#   define  OPMODE_NORMAL       (0 << 3)
#   define  OPMODE_NONDRIVING   (1 << 3)
#   define  OPMODE_DISABLE_BIT_NRZI (2 << 3)
#   define  TERMSELECT      (1 << 2)
#   define  XCVRSELECT_MASK     (3 << 0) /* bits 0 and 1 */
#   define  XCVRSELECT_HS       (0 << 0)
#   define  XCVRSELECT_FS       (1 << 0)
#   define  XCVRSELECT_LS       (2 << 0)
#   define  XCVRSELECT_FS4LS    (3 << 0)
#define IFC_CTRL		0x7
#define IFC_CTRL_SET	0x8
#define IFC_CTRL_CLR	0x9
#   define  INTERFACE_PROTECT_DISABLE (1 << 7)
#   define  AUTORESUME      (1 << 4)
#   define  CLOCKSUSPENDM       (1 << 3)
#   define  CARKITMODE      (1 << 2)
#   define  FSLSSERIALMODE_3PIN (1 << 1)
#define OTG_CTRL		0xa
#define OTG_CTRL_SET	0xb
#define OTG_CTRL_CLR	0xc
#define  DRVVBUS         (1 << 5)
#define  CHRGVBUS        (1 << 4)
#define  DISCHRGVBUS     (1 << 3)
#define  DMPULLDOWN      (1 << 2)
#define  DPPULLDOWN      (1 << 1)
#define  IDPULLUP        (1 << 0)
#define USB_INT_EN_RISE		0xd
#define USB_INT_EN_RISE_SET	0xe
#define USB_INT_EN_RISE_CLR	0xf
#define USB_INT_EN_FALL		0x10
#define USB_INT_EN_FALL_SET	0x11
#define USB_INT_EN_FALL_CLR	0x12
#    define    HOSTDISCONNECT           (1 << 0)
#define USB_INT_STS		0x13
#define USB_INT_LATCH	0x14
#define USB_DEBUG		0x15
#define SCRATCH_REG		0x16
#define SCRATCH_REG_SET	0x17
#define SCRATCH_REG_CLR	0x18
#define CARKIT_CTRL		0x19
#define CARKIT_CTRL_SET	0x1a
#define CARKIT_CTRL_CLR	0x1b
#define        MICEN                    (1 << 6)
#define        SPKRIGHTEN               (1 << 5)
#define        SPKLEFTEN                (1 << 4)
#define        RXDEN                    (1 << 3)
#define        TXDEN                    (1 << 2)
#define        IDGNDDRV                 (1 << 1)
#define        CARKITPWR                (1 << 0)
#define CARKIT_INT_DELAY	0x1c
#define CARKIT_INT_EN		0x1d
#define CARKIT_INT_EN_SET	0x1e
#define CARKIT_INT_EN_CLR	0x1f
#define CARKIT_INT_STS	0x20
#define CARKIT_INT_LATCH	0x21
#define CARKIT_PLS_CTRL		0x22
#define CARKIT_PLS_CTRL_SET	0x23
#define CARKIT_PLS_CTRL_CLR	0x24
#    define    SPKRRIGHT_BIASEN         (1 << 3)
#    define    SPKRLEFT_BIASEN          (1 << 2)
#    define    RXPLSEN                  (1 << 1)
#    define    TXPLSEN                  (1 << 0)
#define TRANS_POS_WIDTH	0x25
#define TRANS_NEG_WIDTH	0x26
#define RCV_PLTY_RECOVERY	0x27
#define MCPC_CTRL		0x30
#define MCPC_CTRL_SET	0x31
#define MCPC_CTRL_CLR	0x32
#define        RTSOL                    (1 << 7)
#define        EXTSWR                   (1 << 6)
#define        EXTSWC                   (1 << 5)
#define        VOICESW                  (1 << 4)
#define        OUT64K                   (1 << 3)
#define        RTSCTSSW                 (1 << 2)
#define        HS_UART                  (1 << 0)
#define MCPC_IO_CTRL 0x033 
#define MCPC_IO_CTRL_SET 0x034 
#define MCPC_IO_CTRL_CLR 0x035 
#define  MICBIASEN       (1<< 5)
#define  CTS_NPU         (1 << 4)
#define  RXD_PU          (1 << 3)
#define  TXDTYP          (1 << 2)
#define  CTSTYP          (1 << 1)
#define  RTSTYP          (1 << 0)
#define MCPC_CTRL2 0x036 
#define MCPC_CTRL2_SET 0x037 
#define MCPC_CTRL2_CLR 0x038 
#	define	MCPC_CK_EN	(1 << 0)
#define OTHER_FUNC_CTRL 0x080 
#define OTHER_FUNC_CTRL_SET 0x081 
#define OTHER_FUNC_CTRL_CLR 0x082 
#define  BDIS_ACON_EN        (1<< 4)
#define  FIVEWIRE_MODE       (1 << 2)
#define OTHER_IFC_CTRL 0x083 
#define OTHER_IFC_CTRL_SET 0x084 
#define OTHER_IFC_CTRL_CLR 0x085 
#    define    OE_INT_EN                (1 << 6)
#    define    CEA2011_MODE             (1 << 5)
#    define    FSLSSERIALMODE_4PIN      (1 << 4)
#    define    HIZ_ULPI_60MHZ_OUT       (1 << 3)
#    define    HIZ_ULPI                 (1 << 2)
#    define    ALT_INT_REROUTE          (1 << 0)
#define OTHER_INT_EN_RISE 0x086 
#define OTHER_INT_EN_RISE_SET 0x087 
#define OTHER_INT_EN_RISE_CLR 0x088 
#define OTHER_INT_EN_FALL 0x089 
#define OTHER_INT_EN_FALL_SET 0x08A 
#define OTHER_INT_EN_FALL_CLR 0x08B 
#define OTHER_INT_STS 0x8C 
#define OTHER_INT_LATCH 0x8D 
#define ID_INT_EN_RISE 0x08E 
#define ID_INT_EN_RISE_SET 0x08F 
#define ID_INT_EN_RISE_CLR 0x090 
#define ID_INT_EN_FALL 0x091 
#define ID_INT_EN_FALL_SET 0x092 
#define ID_INT_EN_FALL_CLR 0x093 
#define ID_INT_STS 0x094 
#define ID_INT_LATCH 0x95 
#define ID_STATUS 0x96 
#define CARKIT_SM_1_INT_EN 0x097 
#define CARKIT_SM_1_INT_EN_SET 0x098 
#define CARKIT_SM_1_INT_EN_CLR 0x099 
#define CARKIT_SM_1_INT_STS 0x09A 
#define CARKIT_SM_1_INT_LATCH 0x9B 
#define CARKIT_SM_2_INT_EN 0x09C 
#define CARKIT_SM_2_INT_EN_SET 0x09D 
#define CARKIT_SM_2_INT_EN_CLR 0x09E 
#define CARKIT_SM_2_INT_STS 0x09F 
#define CARKIT_SM_2_INT_LATCH 0xA0 
#define CARKIT_SM_CTRL 0x0A1 
#define CARKIT_SM_CTRL_SET 0x0A2 
#define CARKIT_SM_CTRL_CLR 0x0A3 
#define CARKIT_SM_CMD 0x0A4 
#define CARKIT_SM_CMD_SET 0x0A5 
#define CARKIT_SM_CMD_CLR 0x0A6 
#define CARKIT_SM_CMD_STS 0xA7 
#define CARKIT_SM_STATUS 0xA8 
#define CARKIT_SM_NEXT_STATUS 0xA9 
#define CARKIT_SM_ERR_STATUS 0xAA 
#define CARKIT_SM_CTRL_STATE 0xAB 
#define POWER_CTRL 0xAC 
#define POWER_CTRL_SET 0xAD 
#define POWER_CTRL_CLR 0xAE 
#   define  OTG_ENAB        (1 << 5)
#define OTHER_IFC_CTRL2 0xAF 
#define OTHER_IFC_CTRL2_SET 0xB0 
#define OTHER_IFC_CTRL2_CLR 0xB1 
#	define	ULPI_TXEN_POL	(1 << 3)
#	define	ULPI_4PIN_2430	(1 << 2)
#define REG_CTRL_EN 0xB2 
#define REG_CTRL_EN_SET 0xB3 
#define REG_CTRL_EN_CLR 0xB4 
#define REG_CTRL_ERROR 0xB5 
#define  ULPI_I2C_CONFLICT_INTEN (1 << 0)
#define OTHER_FUNC_CTRL2 0xB8 
#define OTHER_FUNC_CTRL2_SET 0xB9 
#define OTHER_FUNC_CTRL2_CLR 0xBA 
#define  VBAT_TIMER_EN       (1 << 0)
#define CARKIT_ANA_CTRL 0xBB 
#define CARKIT_ANA_CTRL_SET 0xBC 
#define CARKIT_ANA_CTRL_CLR 0xBD 
#define VBUS_DEBOUNCE 0xC0 
#define ID_DEBOUNCE 0xC1 
#define TPH_DP_CON_MIN 0xC2 
#define TPH_DP_CON_MAX 0xC3 
#define TCR_DP_CON_MIN 0xC4 
#define TCR_DP_CON_MAX 0xC5 
#define TPH_DP_PD_SHORT 0xC6 
#define TPH_CMD_DLY 0xC7 
#define TPH_DET_RST 0xC8 
#define TPH_AUD_BIAS 0xC9 
#define TCR_UART_DET_MIN 0xCA 
#define TCR_UART_DET_MAX 0xCB 
#define TPH_ID_INT_PW 0xCD 
#define TACC_ID_INT_WAIT 0xCE 
#define TACC_ID_INT_PW 0xCF 
#define TPH_CMD_WAIT 0xD0 
#define TPH_ACK_WAIT 0xD1 
#define TPH_DP_DISC_DET 0xD2 
#define VBAT_TIMER 0xD3 
#define CARKIT_4W_DEBUG 0xE0 
#define CARKIT_5W_DEBUG 0xE1 
#define CARKIT_5W_DEBUG 0xE1 
#define TEST_CTRL_CLR 0xEB 
#define TEST_CARKIT_SET 0xEC 
#define TEST_CARKIT_CLR 0xED 
#define TEST_POWER_SET 0xEE 
#define TEST_POWER_CLR 0xEF 
#define TEST_ULPI 0xF0 
#define TXVR_EN_TEST_SET 0xF2 
#define TXVR_EN_TEST_CLR 0xF3 
#define VBUS_EN_TEST 0xF4 
#define ID_EN_TEST 0xF5 
#define PSM_EN_TEST_SET 0xF6 
#define PSM_EN_TEST_CLR 0xF7 
#define PHY_TRIM_CTRL 0xFC 
#define PHY_PWR_CTRL 0xFD 
#   define  PHYPWD          (1 << 0)
#define PHY_CLK_CTRL 0xFE 
#   define  CLOCKGATING_EN      (1 << 2)
#   define  CLK32K_EN           (1 << 1)
#   define  REQ_PHY_DPLL_CLK    (1 << 0)
#define PHY_CLK_CTRL_STS 0xFF 
#   define  PHY_DPLL_CLK        (1 << 0)

#endif

