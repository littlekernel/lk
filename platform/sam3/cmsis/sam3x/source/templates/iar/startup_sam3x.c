/**
 * \file
 *
 * \brief Startup file for SAM3X.
 *
 * Copyright (c) 2011-2012 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */

#include "exceptions.h"
#include "sam3xa.h"
#include "system_sam3x.h"

typedef void (*intfunc) (void);
typedef union { intfunc __fun; void * __ptr; } intvec_elem;

void __iar_program_start(void);
int __low_level_init(void);

/* Exception Table */
#pragma language=extended
#pragma segment="CSTACK"

/* The name "__vector_table" has special meaning for C-SPY: */
/* it is where the SP start value is found, and the NVIC vector */
/* table register (VTOR) is initialized to this address if != 0 */

#pragma section = ".intvec"
#pragma location = ".intvec"
const intvec_elem __vector_table[] = {
	{.__ptr = __sfe("CSTACK")},
	Reset_Handler,

	NMI_Handler,
	HardFault_Handler,
	MemManage_Handler,
	BusFault_Handler,
	UsageFault_Handler,
	(0UL),           /* Reserved */
	(0UL),           /* Reserved */
	(0UL),           /* Reserved */
	(0UL),           /* Reserved */
	SVC_Handler,
	DebugMon_Handler,
	(0UL),           /* Reserved */
	PendSV_Handler,
	SysTick_Handler,

	/* Configurable interrupts */
	SUPC_Handler,    /*  0  Supply Controller */
	RSTC_Handler,    /*  1  Reset Controller */
	RTC_Handler,     /*  2  Real Time Clock */
	RTT_Handler,     /*  3  Real Time Timer */
	WDT_Handler,     /*  4  Watchdog Timer */
	PMC_Handler,     /*  5  PMC */
	EFC0_Handler,    /*  6  EFC 0 */
	EFC1_Handler,    /*  7  EFC 1 */
	UART_Handler,    /*  8  UART */
#ifdef _SAM3XA_SMC_INSTANCE_
	SMC_Handler,     /*  9  SMC */
#else
	(0UL),           /*  9 Reserved */
#endif /* _SAM3XA_SMC_INSTANCE_ */
#ifdef _SAM3XA_SDRAMC_INSTANCE_
	SDRAMC_Handler,  /* 10  SDRAMC */
#else
	(0UL),           /* 10 Reserved */
#endif /* _SAM3XA_SDRAMC_INSTANCE_ */
	PIOA_Handler,    /* 11 Parallel IO Controller A */
	PIOB_Handler,    /* 12 Parallel IO Controller B */
#ifdef _SAM3XA_PIOC_INSTANCE_
	PIOC_Handler,    /* 13 Parallel IO Controller C */
#else
	(0UL),           /* 13 Reserved */
#endif /* _SAM3XA_PIOC_INSTANCE_ */
#ifdef _SAM3XA_PIOD_INSTANCE_
	PIOD_Handler,    /* 14 Parallel IO Controller D */
#else
	(0UL),           /* 14 Reserved */
#endif /* _SAM3XA_PIOD_INSTANCE_ */
#ifdef _SAM3XA_PIOE_INSTANCE_
	PIOE_Handler,    /* 15 Parallel IO Controller E */
#else
	(0UL),           /* 15 Reserved */
#endif /* _SAM3XA_PIOE_INSTANCE_ */
#ifdef _SAM3XA_PIOF_INSTANCE_
	PIOF_Handler,    /* 16 Parallel IO Controller F */
#else
	(0UL),           /* 16 Reserved */
#endif /* _SAM3XA_PIOF_INSTANCE_ */
	USART0_Handler,  /* 17 USART 0 */
	USART1_Handler,  /* 18 USART 1 */
	USART2_Handler,  /* 19 USART 2 */
#ifdef _SAM3XA_USART3_INSTANCE_
	USART3_Handler,  /* 20 USART 3 */
#else
	(0UL),           /* 20 Reserved */
#endif /* _SAM3XA_USART3_INSTANCE_ */
	HSMCI_Handler,   /* 21 MCI */
	TWI0_Handler,    /* 22 TWI 0 */
	TWI1_Handler,    /* 23 TWI 1 */
	SPI0_Handler,    /* 24 SPI 0 */
#ifdef _SAM3XA_SPI1_INSTANCE_
	SPI1_Handler,    /* 25 SPI 1 */
#else
	(0UL),           /* 25 Reserved */
#endif /* _SAM3XA_SPI1_INSTANCE_ */
	SSC_Handler,     /* 26 SSC */
	TC0_Handler,     /* 27 Timer Counter 0 */
	TC1_Handler,     /* 28 Timer Counter 1 */
	TC2_Handler,     /* 29 Timer Counter 2 */
	TC3_Handler,     /* 30 Timer Counter 3 */
	TC4_Handler,     /* 31 Timer Counter 4 */
	TC5_Handler,     /* 32 Timer Counter 5 */
#ifdef _SAM3XA_TC2_INSTANCE_
	TC6_Handler,     /* 33 Timer Counter 6 */
	TC7_Handler,     /* 34 Timer Counter 7 */
	TC8_Handler,     /* 35 Timer Counter 8 */
#else
	(0UL),           /* 33 Reserved */
	(0UL),           /* 34 Reserved */
	(0UL),           /* 35 Reserved */
#endif /* _SAM3XA_TC2_INSTANCE_ */
	PWM_Handler,     /* 36 PWM */
	ADC_Handler,     /* 37 ADC controller */
	DACC_Handler,    /* 38 DAC controller */
	DMAC_Handler,    /* 39 DMA Controller */
	UOTGHS_Handler,  /* 40 USB OTG High Speed */
	TRNG_Handler,    /* 41 True Random Number Generator */
#ifdef _SAM3XA_EMAC_INSTANCE_
	EMAC_Handler,    /* 42 Ethernet MAC */
#else
	(0UL),           /* 42 Reserved */
#endif /* _SAM3XA_EMAC_INSTANCE_ */
	CAN0_Handler,    /* 43 CAN Controller 0 */
	CAN1_Handler     /* 44 CAN Controller 1 */
};

/* EWARM 6.30 integrates CMSIS 2.10 (__CM3_CMSIS_VERSION 0x0210),
 * in which SCB_VTOR_TBLBASE_Msk not defined.
 */
#if (__VER__ >= 6030000)
/* TEMPORARY PATCH FOR SCB */
#define SCB_VTOR_TBLBASE_Pos               29                            /*!< SCB VTOR: TBLBASE Position */
#define SCB_VTOR_TBLBASE_Msk               (1UL << SCB_VTOR_TBLBASE_Pos) /*!< SCB VTOR: TBLBASE Mask */
#endif

/**------------------------------------------------------------------------------
 * This is the code that gets called on processor reset. To initialize the
 * device.
 *------------------------------------------------------------------------------*/
int __low_level_init(void)
{
	uint32_t *pSrc = __section_begin(".intvec");

	SCB->VTOR = ((uint32_t) pSrc & SCB_VTOR_TBLOFF_Msk);

	if (((uint32_t) pSrc >= IRAM0_ADDR) && ((uint32_t) pSrc < NFC_RAM_ADDR)) {
		SCB->VTOR |= 1 << SCB_VTOR_TBLBASE_Pos;
	}

	return 1; /* if return 0, the data sections will not be initialized */
}

/**------------------------------------------------------------------------------
 * This is the code that gets called on processor reset. To initialize the
 * device.
 *------------------------------------------------------------------------------*/
void Reset_Handler(void)
{
	__iar_program_start();
}
