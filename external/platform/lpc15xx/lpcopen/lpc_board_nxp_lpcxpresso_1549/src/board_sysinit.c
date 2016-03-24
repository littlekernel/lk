/*
 * @brief LPCXPresso LPC1549 Sysinit file
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2014
 * All rights reserved.
 *
 * @par
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * LPC products.  This software is supplied "AS IS" without any warranties of
 * any kind, and NXP Semiconductors and its licensor disclaim any and
 * all warranties, express or implied, including all implied warranties of
 * merchantability, fitness for a particular purpose and non-infringement of
 * intellectual property rights.  NXP Semiconductors assumes no responsibility
 * or liability for the use of the software, conveys no license or rights under any
 * patent, copyright, mask work right, or any other intellectual property rights in
 * or to any products. NXP Semiconductors reserves the right to make changes
 * in the software without notification. NXP Semiconductors also makes no
 * representation or warranty that such application will be suitable for the
 * specified use without further testing or modification.
 *
 * @par
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors' and its
 * licensor's relevant copyrights in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 */

 #include "board.h"
 #include "string.h"

/* The System initialization code is called prior to the application and
   initializes the board for run-time operation. Board initialization
   includes clock setup and default pin muxing configuration. */

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/* IOCON setup table, only items that need changing from their default pin
   state are in this table. */
STATIC const PINMUX_GRP_T ioconSetup[] = {
	/* LEDs */
	{0, 25,  (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},		/* PIO0_25-BREAK_CTRL-RED (low enable) */
	{0, 3,   (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},		/* PIO0_3-SCT1_OUT4-GRN */
	{1, 1,   (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},		/* PIO1_1-BREAK_STS1-BLUE */

	/* QEI, motor controler, I2C, CAN */
	{0, 2,   (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},		/* PIO0_2-QEI-SCT0_IN */
	{0, 30,  (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},		/* PIO0_30-QEI-SCT0_IN */
	{0, 17,  (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},		/* PIO0_17-QEI-SCT0_IN */
	{0, 25,  (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},		/* PIO0_25-BREAK_CTRL-RED */
	{1, 1,   (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},		/* PIO1_1-BREAK_STS1-BLUE */
	{0, 23,  (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},		/* PIO0_23-I2C_SDA */
	{0, 22,  (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},		/* PIO0_22-I2C_SCL */
	{0, 11,  (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},		/* PIO0_11-CAN_RD */
	{0, 31,  (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},		/* PIO0_31-CAN_TD */

	/* ADC */
	{1, 3,   (IOCON_MODE_INACT)},							/* PIO1_3-ADC1_5 */
	{0, 4,   (IOCON_MODE_INACT)},							/* PIO0_4-ADC0_4 */
	{0, 5,   (IOCON_MODE_INACT)},							/* PIO0_5-ADC0_3 */
	{0, 7,   (IOCON_MODE_INACT)},							/* PIO0_7-ADC0_1 */
	{0, 8,   (IOCON_MODE_INACT)},							/* PIO0_8-ADC0_0 */
	{0, 9,   (IOCON_MODE_INACT)},							/* PIO0_9-ADC1_1 */
	{0, 10,  (IOCON_MODE_INACT)},							/* PIO0_10-ADC1_2 */

	/* Joystick */
	{1, 4,   (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},		/* PIO1_4-JOY_U */
	{1, 5,   (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},		/* PIO1_5-JOY_C */
	{1, 6,   (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},		/* PIO1_6-JOY_D */
	{1, 7,   (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},		/* PIO1_7-JOY_R */
	{1, 8,   (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},		/* PIO1_8-JOY_L */

	/* UART */
	{0, 13,  (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},		/* PIO0_13-ISP_RX */
	{0, 18,  (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},		/* PIO0_18-ISP_TX */
	{0, 11,  (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},
	{0, 31,  (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},

	/* USB related */
	{1, 11,  (IOCON_MODE_PULLDOWN | IOCON_DIGMODE_EN)},	/* PIO1_11-ISP_1 (VBUS) */
};

/* SWIM pin assignment definitions for pin assignment/muxing */
typedef struct {
	uint16_t assignedpin : 9;		/* Function and mode */
	uint16_t port : 2;				/* Pin port */
	uint16_t pin : 5;				/* Pin number */
} SWM_GRP_T;

/* Pin muxing table, only items that need changing from their default pin
   state are in this table. */
STATIC const SWM_GRP_T swmSetup[] = {
	/* USB related */
	{(uint16_t) SWM_USB_VBUS_I, 1, 11},		/* PIO1_11-ISP_1-AIN_CTRL */

	/* UART */
	{(uint16_t) SWM_UART0_RXD_I, 0, 13},		/* PIO0_13-ISP_RX */
	{(uint16_t) SWM_UART0_TXD_O, 0, 18},		/* PIO0_18-ISP_TX */
};

/* Setup fixed pin functions (GPIOs are fixed) */
/* No fixed pins except GPIOs */
#define PINENABLE0_VAL 0xFFFFFFFF

/* No fixed pins except GPIOs */
#define PINENABLE1_VAL 0x00FFFFFF

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/* Sets up system pin muxing */
void Board_SetupMuxing(void)
{
	int i;

	/* Enable SWM and IOCON clocks */
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_IOCON);
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_SWM);
	Chip_SYSCTL_PeriphReset(RESET_IOCON);

	/* IOCON setup */
	Chip_IOCON_SetPinMuxing(LPC_IOCON, ioconSetup, sizeof(ioconSetup) / sizeof(PINMUX_GRP_T));

	/* SWM assignable pin setup */
	for (i = 0; i < (sizeof(swmSetup) / sizeof(SWM_GRP_T)); i++) {
		Chip_SWM_MovablePortPinAssign((CHIP_SWM_PIN_MOVABLE_T) swmSetup[i].assignedpin,
									  swmSetup[i].port, swmSetup[i].pin);
	}

	/* SWM fixed pin setup */
	//	LPC_SWM->PINENABLE[0] = PINENABLE0_VAL;
	//	LPC_SWM->PINENABLE[1] = PINENABLE1_VAL;

	/* Note SWM and IOCON clocks are left on */
}

/* Set up and initialize clocking prior to call to main */
void Board_SetupClocking(void)
{
	Chip_SetupXtalClocking();

	/* Set USB PLL input to main oscillator */
	Chip_Clock_SetUSBPLLSource(SYSCTL_PLLCLKSRC_MAINOSC);
	/* Setup USB PLL  (FCLKIN = 12MHz) * 4 = 48MHz
	   MSEL = 3 (this is pre-decremented), PSEL = 1 (for P = 2)
	   FCLKOUT = FCLKIN * (MSEL + 1) = 12MHz * 4 = 48MHz
	   FCCO = FCLKOUT * 2 * P = 48MHz * 2 * 2 = 192MHz (within FCCO range) */
	Chip_Clock_SetupUSBPLL(3, 1);

	/* Powerup USB PLL */
	Chip_SYSCTL_PowerUp(SYSCTL_POWERDOWN_USBPLL_PD);

	/* Wait for PLL to lock */
	while (!Chip_Clock_IsUSBPLLLocked()) {}

	/* Set default system tick divder to 1 */
	Chip_Clock_SetSysTickClockDiv(1);
}

/* Set up and initialize hardware prior to call to main */
void Board_SystemInit(void)
{
	/* Setup system clocking and muxing */
	Board_SetupMuxing();
	Board_SetupClocking();

	/* Set SYSTICKDIV to 1 so CMSIS Systick functions work */
	LPC_SYSCTL->SYSTICKCLKDIV = 1;
}
