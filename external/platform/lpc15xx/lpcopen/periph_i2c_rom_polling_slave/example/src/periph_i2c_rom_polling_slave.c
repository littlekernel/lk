/*
 * @brief I2C bus slave polling example using the ROM API
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
static I2C_HANDLE_T *i2cHandleSlave;

/* Use a buffer size larger than the expected return value of
   i2c_get_mem_size() for the static I2C handle type */
static uint32_t i2cSlaveHandleMEM[0x20];

/* Since I2C uses the most-significant 7-bits of each byte-lane
   this address represents device 0x22. 4 addresses are supported. */
#define I2C_ADDR_7BIT               (0x00000048)

/* This mask will only allow device 0x22. */
#define I2C_ADDR_7BIT_MASK          (0x00000000)

/* Standard I2C mode */
#define I2C_MODE    (0)

/* Receive and transmit buffers */
static uint8_t recvBuff[16], tranBuff[16];

/* Global I2C ROM API parameter and results structures */
static I2C_PARAM_T paramRX, paramTX;
static I2C_RESULT_T resultRX, resultTX;

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

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
static void errorI2C()
{
	Board_LED_Set(0, true);
	while (1) {}
}

/* Setup I2C handle and parameters */
static void setupI2CSlave()
{
	ErrorCode_t error_code;

	/* Enable I2C clock and reset I2C peripheral - the boot ROM does not
	   do this */
	Chip_I2C_Init(LPC_I2C0);

	/* Perform a sanity check on the storage allocation */
	if (LPC_I2CD_API->i2c_get_mem_size() > sizeof(i2cSlaveHandleMEM)) {
		/* Example only: this should never happen and probably isn't needed for
		   most I2C code. */
		errorI2C();
	}

	/* Setup the I2C handle */
	i2cHandleSlave = LPC_I2CD_API->i2c_setup(LPC_I2C_BASE, i2cSlaveHandleMEM);
	if (i2cHandleSlave == NULL) {
		errorI2C();
	}

	/* Set a single 7-bit I2C address, only 7-bit addressing is supported,
			but the ROM API expects the address byte value */
	error_code = LPC_I2CD_API->i2c_set_slave_addr(i2cHandleSlave,
												  I2C_ADDR_7BIT << 1, I2C_ADDR_7BIT_MASK);
	if (error_code != LPC_OK) {
		DEBUGOUT("Error setting I2C slave address\r\n");
		errorI2C();
	}

	/* No need to set I2C clock rate in slave mode */
}

/* Slave transmit in polling mode */
static void sendI2CSlave(void)
{
	ErrorCode_t error_code;

	/* Send 1 byte based on master request */
	paramTX.num_bytes_send = 1;
	paramTX.buffer_ptr_send = &tranBuff[0];

	/* Polling function is blocking */
	error_code = LPC_I2CD_API->i2c_slave_transmit_poll(i2cHandleSlave, &paramTX, &resultTX);

	/* Completed without erors? */
	if (error_code != LPC_OK) {
		DEBUGOUT("i2c_slave_transmit_poll error code : %x\r\b", error_code);
		errorI2C();
	}
}

/* Slave receive in polling mode */
static void readI2CSlave(void)
{
	ErrorCode_t error_code;
	bool ledState;

	/* Setup receive buffer, receive buffer size, and receive callback */
	paramRX.num_bytes_rec = 2;	/* Address and single byte */
	paramRX.buffer_ptr_rec = &recvBuff[0];

	/* Function is blocking */
	error_code = LPC_I2CD_API->i2c_slave_receive_poll(i2cHandleSlave, &paramRX, &resultRX);

	/* Completed without erors? */
	if (error_code != LPC_OK) {
		DEBUGOUT("i2c_slave_receive_poll error code : %x\r\b", error_code);
	}
	else {
			/* Setup LED on/off state */
			ledState = (bool) recvBuff[1];

			/* Turn LED on = 1 or off = 0 */
			Board_LED_Set(0, ledState);

			/* Toggle LED state on next master read */
			tranBuff[0] = 1 - recvBuff[1];
			/* Transmit toggled LED state */
			sendI2CSlave();
	}
}

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/**
 * @brief	Main routine for I2C example
 * @return	Function should not exit
 */
int main(void)
{
	/* Generic Initialization */
	SystemCoreClockUpdate();
	Board_Init();

	/* Set initial LED state to off */
	Board_LED_Set(0, false);

	/* Setup I2C at the board level (usually pin muxing) */
	Init_I2C_PinMux();

	/* Allocate I2C handle, setup I2C rate, and initialize I2C
	   clocking */
	setupI2CSlave();

	/* I2C slave handler loop - loop forever waiting for requests from
	   master to receive or send data */
	while (1) {
		/* Poll until the next read/write operation */
		readI2CSlave();
	}

	return 0;
}
