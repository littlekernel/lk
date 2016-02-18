/*
 * @brief I2CM bus slave example using interrupt mode
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

/* I2CS transfer record for master and slave operations */
static I2CM_XFER_T  i2cmXferRec;

/* I2C clock is set to 1.8MHz */
#define I2C_CLK_DIVIDER     (40)

/* 100KHz I2C bit-rate - going too fast may prevent the salev from responding
   in time */
#define I2C_BITRATE         (100000)
/* Standard I2C mode */
#define I2C_MODE    (0)

/* Emulated EEPROM slave addresses */
#define EEPROM0SLVADD       (0x28)
#define EEPROM1SLVADD       (0x2C)

#if defined(BOARD_NXP_LPCXPRESSO_1549)
/** Our slave address and I2C information */
#define LPC_I2C_PORT         LPC_I2C0
#define LPC_I2C_INTHAND      I2C0_IRQHandler
#define LPC_IRQNUM           I2C0_IRQn
#endif

/* Emulated EEPROM device2 - size, buffer, and current address */
#define EMUEEPROMSIZE 512
static uint8_t eepromData[2][EMUEEPROMSIZE];
static uint16_t eepromAddr[2];
static int curEEPROM, addrbytes;

/* work buffers for this example */
uint8_t txWorkBuff[EMUEEPROMSIZE], rxWorkBuff[EMUEEPROMSIZE];

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
	/* Configure your own I2C pin muxing here if needed */
#error "No I2C Pin Muxing defined for this example"
#endif
}

/* Setup I2C */
static void setupI2CMaster(void)
{
	/* Enable I2C clock and reset I2C peripheral */
	Chip_I2C_Init(LPC_I2C_PORT);

	/* Setup clock rate for I2C */
	Chip_I2C_SetClockDiv(LPC_I2C_PORT, I2C_CLK_DIVIDER);

	/* Setup I2CM transfer rate */
	Chip_I2CM_SetBusSpeed(LPC_I2C_PORT, I2C_BITRATE);

	/* Enable I2C master interface */
	Chip_I2CM_Enable(LPC_I2C_PORT);
}

/* Setup I2C */
static void setupI2CSlave(void)
{
	/* Some common I2C init was performed in setupI2CMaster(), so it doesn't
	   need to be done again for the slave setup. */

	/* Emulated EEPROM 0 is on slave index 0 */
	Chip_I2CS_SetSlaveAddr(LPC_I2C_PORT, 0, EEPROM0SLVADD);
	/* Disable Qualifier for Slave Address 0 */
	Chip_I2CS_SetSlaveQual0(LPC_I2C_PORT, false, 0);
	/* Enable Slave Address 0 */
	Chip_I2CS_EnableSlaveAddr(LPC_I2C_PORT, 0);

	/* Emulated EEPROM 1 is on slave index 1 */
	Chip_I2CS_SetSlaveAddr(LPC_I2C_PORT, 1, EEPROM1SLVADD);
	/* Enable Slave Address 1 */
	Chip_I2CS_EnableSlaveAddr(LPC_I2C_PORT, 1);

	/* Clear interrupt status and enable slave interrupts */
	Chip_I2CS_ClearStatus(LPC_I2C_PORT, I2C_STAT_SLVDESEL);
	Chip_I2C_EnableInt(LPC_I2C_PORT, I2C_INTENSET_SLVPENDING | I2C_INTENSET_SLVDESEL);

	/* Enable I2C slave interface */
	Chip_I2CS_Enable(LPC_I2C_PORT);
}

/* Setup emulated EEPROM */
static void setupEmuEEPROM(void)
{
	eepromAddr[0] = 0;
	eepromAddr[1] = 0;
}

/* Set emulated EEPROM address */
static void setEmuEEPROMAddr(int eepromNum, uint16_t addr)
{
	eepromAddr[eepromNum] = (addr & (EMUEEPROMSIZE - 1));
}

/* Increment emulated EEPROM address */
static void incEmuEEPROMAddr(int eepromNum)
{
	eepromAddr[eepromNum]++;
	if (eepromAddr[eepromNum] >= EMUEEPROMSIZE) {
		eepromAddr[eepromNum] = 0;
	}
}

/* Read a byte from the emulated EEPROM and incrmenet address */
static uint8_t readEmuEEPROM(int eepromNum)
{
	uint8_t data = eepromData[eepromNum][eepromAddr[eepromNum]];

	incEmuEEPROMAddr(eepromNum);

	return data;
}

/* Write a byte to the emulated EEPROM and incrmenet address */
static void writeEmuEEPROM(int eepromNum, uint8_t data)
{
	eepromData[eepromNum][eepromAddr[eepromNum]] = data;

	incEmuEEPROMAddr(eepromNum);
}

/* Function to wait for I2CM transfer completion */
static void WaitForI2cXferComplete(I2CM_XFER_T *xferRecPtr)
{
	/* Test for still transferring data */
	while (xferRecPtr->status == I2CM_STATUS_BUSY) {
		/* Sleep until next interrupt */
		__WFI();
	}
}

/* Function to setup and execute I2C transfer request */
static void SetupXferRecAndExecute(uint8_t devAddr,
								   uint8_t *txBuffPtr,
								   uint16_t txSize,
								   uint8_t *rxBuffPtr,
								   uint16_t rxSize)
{
	/* Setup I2C transfer record */
	i2cmXferRec.slaveAddr = devAddr;
	i2cmXferRec.status = 0;
	i2cmXferRec.txSz = txSize;
	i2cmXferRec.rxSz = rxSize;
	i2cmXferRec.txBuff = txBuffPtr;
	i2cmXferRec.rxBuff = rxBuffPtr;

	/* Wait for master to go pending - needed in mixed master/slave mode on single I2C bus */
	while (Chip_I2CM_IsMasterPending(LPC_I2C_PORT) == false) {}

	Chip_I2CM_Xfer(LPC_I2C_PORT, &i2cmXferRec);
	/* Enable Master Interrupts */
	Chip_I2C_EnableInt(LPC_I2C_PORT, I2C_INTENSET_MSTPENDING | I2C_INTENSET_MSTRARBLOSS | I2C_INTENSET_MSTSTSTPERR);
	/* Wait for transfer completion */
	WaitForI2cXferComplete(&i2cmXferRec);
	/* Disable all Interrupts */
	Chip_I2C_DisableInt(LPC_I2C_PORT, I2C_INTENSET_MSTPENDING | I2C_INTENSET_MSTRARBLOSS | I2C_INTENSET_MSTSTSTPERR);

	if (i2cmXferRec.status != I2CM_STATUS_OK) {
		DEBUGOUT("\r\nI2C error: %d\r\n", i2cmXferRec.status);
	}
}

/* Handler for slave start callback */
static void processSlaveTransferStart(uint8_t addr)
{
	if (addr == EEPROM0SLVADD) {
		curEEPROM = 0;
	}
	else {
		curEEPROM = 1;
	}

	addrbytes = 0;
}

/* Handler for slave send callback */
static uint8_t processSlaveTransferSend(uint8_t *data)
{
	/* Send data from emulated EEPROM */
	*data = readEmuEEPROM(curEEPROM);

	return 0;
}

/* Handler for slave receive callback */
static uint8_t processSlaveTransferRecv(uint8_t data)
{
	static uint16_t addr;

	if (addrbytes == 0) {
		/* Address MSB bytes is in */
		addr = ((uint16_t) data) << 8;
	}
	else if (addrbytes == 1) {
		/* Address LSB bytes is in */
		addr |= (uint16_t) data;

		/* Set emulated EEPROM address */
		setEmuEEPROMAddr(curEEPROM, addr);
	}
	else {
		/* Write data to emulated EEPROM */
		writeEmuEEPROM(curEEPROM, data);
	}
	addrbytes++;

	return 0;
}

/* Handler for slave transfer complete callback */
static void processSlaveTransferDone(void)
{
	/* Nothing needs to be done here */
}

/* I2C slavecallback function list */
const static I2CS_XFER_T i2csCallBacks = {
	&processSlaveTransferStart,
	&processSlaveTransferSend,
	&processSlaveTransferRecv,
	&processSlaveTransferDone
};

/* Populate EEPROM with some initial data */
static void fillWorkBuff(uint8_t *buff, uint8_t seed)
{
	int i;

	for (i = 0; i < EMUEEPROMSIZE; i++) {
		buff[i] = seed;
		seed = seed + (uint8_t) i;
	}
}

/* Convert a 16-bit address to to 8-bit values */
static void toAddr(uint8_t *tx, uint16_t addr)
{
	tx[0] = (uint8_t) (addr >> 8);
	tx[1] = (uint8_t) (addr >> 0);
}

/* Compate 2 fixed length buffers */
static bool compFail(uint8_t *ptr1, uint8_t *ptr2)
{
	int i;

	for (i  = 0; i < EMUEEPROMSIZE; i++) {
		if (ptr1[i] != ptr2[i]) {
			return true;
		}
	}

	return false;
}

/* EEPROM testing via I2C */
static void eepromTest(uint8_t eepromAddr)
{
	int i;
	uint8_t rx[8], tx[8];
	static uint8_t seed = 0;

	DEBUGOUT("Testing emulated EEPROM @ %x : ", eepromAddr);

	/* Generate data to send to the emulated EEPROM */
	fillWorkBuff(txWorkBuff, seed);
	seed += 33 + eepromAddr;

	/* Write the first 128 bytes to the EEPROM with byte operations */
	for (i = 0; i < 128; i++) {
		toAddr(tx, (uint16_t) i);
		tx[2] = txWorkBuff[i];
		SetupXferRecAndExecute(eepromAddr, tx, 3, rx, 0);
	}

	/* Write the next 128 bytes as 16-bit operations */
	for (i = 0; i < 128; i += 2) {
		toAddr(tx, (128 + (uint16_t) i));
		tx[2] = txWorkBuff[128 + i];
		tx[3] = txWorkBuff[128 + i + 1];
		SetupXferRecAndExecute(eepromAddr, tx, 4, rx, 0);
	}

	/* Write the next 256 bytes as 32-bit operations */
	for (i = 0; i < 256; i += 4) {
		toAddr(tx, (256 + (uint16_t) i));
		tx[2] = txWorkBuff[256 + i];
		tx[3] = txWorkBuff[256 + i + 1];
		tx[4] = txWorkBuff[256 + i + 2];
		tx[5] = txWorkBuff[256 + i + 3];
		SetupXferRecAndExecute(eepromAddr, tx, 6, rx, 0);
	}

	/* Read back 128 individual bytes */
	for (i = 0; i < 128; i++) {
		toAddr(tx, (uint16_t) i);
		SetupXferRecAndExecute(eepromAddr, tx, 2, rx, 1);
		rxWorkBuff[i] = rx[0];
	}

	/* Read the next 128 bytes as 16-bit operations */
	for (i = 0; i < 128; i += 2) {
		toAddr(tx, (128 + (uint16_t) i));
		SetupXferRecAndExecute(eepromAddr, tx, 2, rx, 2);
		rxWorkBuff[128 + i] = rx[0];
		rxWorkBuff[128 + i + 1] = rx[1];
	}

	/* Read the next 256 bytes as 32-bit operations */
	for (i = 0; i < 256; i += 4) {
		toAddr(tx, (256 + (uint16_t) i));
		SetupXferRecAndExecute(eepromAddr, tx, 2, rx, 4);
		rxWorkBuff[256 + i] = rx[0];
		rxWorkBuff[256 + i + 1] = rx[1];
		rxWorkBuff[256 + i + 2] = rx[2];
		rxWorkBuff[256 + i + 3] = rx[3];
	}

	/* Compare send and receive data */
	if (compFail(rxWorkBuff, txWorkBuff)) {
		Board_LED_Set(0, true);
		DEBUGOUT("FAILED\r\n");
	}
	else {
		DEBUGOUT("PASSED\r\n");
	}
}

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/**
 * @brief	Handle I2C1 interrupt by calling I2CM interrupt transfer handler
 * @return	Nothing
 */
void LPC_I2C_INTHAND(void)
{
	uint32_t state = Chip_I2C_GetPendingInt(LPC_I2C_PORT);

	/* Error handling */
	if (state & (I2C_INTSTAT_MSTRARBLOSS | I2C_INTSTAT_MSTSTSTPERR)) {
		Chip_I2CM_ClearStatus(LPC_I2C_PORT, I2C_STAT_MSTRARBLOSS | I2C_STAT_MSTSTSTPERR);
	}

	/* Call I2CM ISR function with the I2C device and transfer rec */
	if (state & I2C_INTENSET_MSTPENDING) {
		Chip_I2CM_XferHandler(LPC_I2C_PORT, &i2cmXferRec);
	}

	/* I2C slave related interrupt */
	while (state & (I2C_INTENSET_SLVPENDING | I2C_INTENSET_SLVDESEL)) {
		Chip_I2CS_XferHandler(LPC_I2C_PORT, &i2csCallBacks);

		/* Update state */
		state = Chip_I2C_GetPendingInt(LPC_I2C_PORT);
	}
}

/**
 * @brief	Main routine for I2C example
 * @return	Function should not exit
 */
int main(void)
{
	/* Generic Initialization */
	SystemCoreClockUpdate();
	Board_Init();

	/* Clear activity LED */
	Board_LED_Set(0, false);

	/* Setup I2C pin muxing */
	Init_I2C_PinMux();

	/* Setup I2C, master, and slave */
	setupI2CMaster();
	setupI2CSlave();

	/* Setup emulated EEPROMd evice */
	setupEmuEEPROM();

	/* Enable the interrupt for the I2C */
	NVIC_EnableIRQ(LPC_IRQNUM);

	DEBUGOUT("2 Emulated EEPROM I2C devices using 2 I2C slaves\r\n");

	/* Test each emulated EEPROM */
	while (1) {
		/* Read some data from the emulated EEPROMs */
		eepromTest(EEPROM0SLVADD);
		eepromTest(EEPROM1SLVADD);
	}

	/* Code never reaches here. Only used to satisfy standard main() */
	return 0;
}
