/*
 * @brief I2C bus master example using the ROM API in polling mode
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

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/* I2C master handle and memory for ROM API */
static I2C_HANDLE_T *i2cHandleMaster;

/* Use a buffer size larger than the expected return value of
   i2c_get_mem_size() for the static I2C handle type */
static uint32_t i2cMasterHandleMEM[0x20];

#define SPEED_100KHZ                (100000)
#define I2C_RD_CMD_BIT      (0x01)
/* Standard I2C mode */
#define I2C_MODE    (0)

/* 7-bit I2C addresses */
#define I2C_ADDR_7BIT       (0x90)		/* This is the 7-bit address shifted up 1-bit (orig 0x48) */

/* SysTick rate in Hz */
#define TICKRATE_HZ         (10)

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/* Current state for LED control via I2C cases */
static volatile int state;

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/* Initializes pin muxing for I2C interface - note that SystemInit() may
   already setup your pin muxing at system startup */
static void Init_I2C_PinMux(void)
{
#if defined(BOARD_NXP_LPCXPRESSO_1549)
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 22, IOCON_DIGMODE_EN | I2C_MODE);
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 23, IOCON_DIGMODE_EN | I2C_MODE);
	Chip_SWM_EnableFixedPin(SWM_FIXED_I2C0_SCL);
	Chip_SWM_EnableFixedPin(SWM_FIXED_I2C0_SDA);
#else
#error "No I2C Pin Muxing defined for this example"
#endif
}

/* Turn on LED to indicate an error */
static void errorI2C(void)
{
	Board_LED_Set(0, true);
	while (1) {}
}

/* Setup I2C handle and parameters */
static void setupI2CMaster()
{
	/* Enable I2C clock and reset I2C peripheral - the boot ROM does not
	   do this */
	Chip_I2C_Init(LPC_I2C0);

	/* Perform a sanity check on the storage allocation */
	if (LPC_I2CD_API->i2c_get_mem_size() > sizeof(i2cMasterHandleMEM)) {
		/* Example only: this should never happen and probably isn't needed for
		   most I2C code. */
		errorI2C();
	}

	/* Setup the I2C handle */
	i2cHandleMaster = LPC_I2CD_API->i2c_setup(LPC_I2C_BASE, i2cMasterHandleMEM);
	if (i2cHandleMaster == NULL) {
		errorI2C();
	}

	/* Set I2C bitrate */
	if (LPC_I2CD_API->i2c_set_bitrate(i2cHandleMaster, Chip_Clock_GetSystemClockRate(),
									  SPEED_100KHZ) != LPC_OK) {
		errorI2C();
	}
}

/* Master receive in interrupt mode */
static void readI2CMaster(uint16_t AddressI2C, uint8_t *readPtr, bool address10Bit)
{
	uint8_t recvData[10];
	I2C_PARAM_T param;
	I2C_RESULT_T result;
	ErrorCode_t error_code;
	uint8_t rx_index = 0;
	uint8_t txData[3];
	uint8_t tx_index = 0;

	/* Setup I2C receive for address/read, read desired LED state, then stop */
	if (address10Bit) {
		/* 10-bit addressing - 4 MSBs of slave address in first byte of
		   transmit buffer */
		recvData[rx_index++] = (uint8_t) (((AddressI2C >> 7) & 0x06) | 0xF0);
		recvData[rx_index++] = (uint8_t) (AddressI2C & 0x0FF);
		
		txData[tx_index++] = (uint8_t) (((AddressI2C >> 7) & 0x06) | 0xF0);
		txData[tx_index++] = (uint8_t) (AddressI2C & 0x0FF);
	}
	else {
		/* 7-bit address */
		recvData[rx_index++] = (uint8_t) AddressI2C;
		txData[tx_index++] = (uint8_t) AddressI2C;
	}

	/* Setup I2C parameters for number of bytes with stop - appears as follows on bus:
	   Start - address7 or address10upper - ack
	   (10 bits addressing only) address10lower - ack
	   value 1 (read) - ack
	   value 2 read) - ack - stop */
	txData[tx_index++] = 0;
	param.num_bytes_send = tx_index;
	param.num_bytes_rec = rx_index + 2;
	param.buffer_ptr_send = &txData[0];
	param.buffer_ptr_rec = &recvData[0];
	param.stop_flag = 1;

	/* Set timeout (much) greater than the transfer length */
	LPC_I2CD_API->i2c_set_timeout(i2cHandleMaster, 100000);

	/* Function is non-blocking, returned error should be LPC_OK, but isn't checked here */
	error_code = LPC_I2CD_API->i2c_master_tx_rx_poll(i2cHandleMaster, &param, &result);

	/* Completed without erors? */
	if (error_code != LPC_OK) {
		/* Likely cause is NAK */
		DEBUGOUT("i2c_master_receive error code : %x\r\b", error_code);
		errorI2C();
	}

	/* Note results are only valid when there are no errors */
	*readPtr++ = recvData[1];
	*readPtr++ = recvData[2];
}

/* Read current temperature from LM75A part over I2C */
static void ReadTemperatureI2C(void)
{
	int8_t readState_Input[2];

	/* Read temperature */
	readI2CMaster(I2C_ADDR_7BIT | I2C_RD_CMD_BIT, (uint8_t *) readState_Input, false);
	/* Output temperature. */
	DEBUGOUT("Temperature read over I2C is %d Celsius\r\n",
			 (((int16_t) readState_Input[0] << 3) | ((uint8_t) readState_Input[1] >> 5)) / 8);
}

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/**
 * @brief	Handle interrupt from SysTick timer
 * @return	Nothing
 */
void SysTick_Handler(void)
{
	static int ticks = 0;

	ticks++;
	if (ticks > TICKRATE_HZ) {
		ticks = 0;
		state = 1 - state;
	}
}

/**
 * @brief	Main routine for I2C example
 * @return	Function should not exit
 */
int main(void)
{
	int lastState = 0;
	state = 0;

	/* Generic Initialization */
	SystemCoreClockUpdate();
	Board_Init();

	/* Clear activity LED */
	Board_LED_Set(0, false);

	/* Setup I2C pin muxing */
	Init_I2C_PinMux();

	/* Allocate I2C handle, setup I2C rate, and initialize I2C
	   clocking */
	setupI2CMaster();

	/* Enable SysTick Timer */
	SysTick_Config(SystemCoreClock / TICKRATE_HZ);

	/* Loop forever, toggle LED on other board via I2C */
	while (1) {

		/* Sleep until a state change occurs in SysTick */
		while (lastState == state) {
			__WFI();
		}

		/* Read current temperature from LM75A part over I2C */
		ReadTemperatureI2C();
		
		/* Reset lastState to allow for WFI */
		lastState = state;

		/* Toggle LED to show activity. */
		Board_LED_Toggle(0);
	}

	/* Code never reaches here. Only used to satisfy standard main() */
	return 0;
}
