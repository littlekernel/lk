/*
 * @brief LPC15xx Power ROM API declarations and functions
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2013
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

#ifndef __ROM_PWR_15XX_H_
#define __ROM_PWR_15XX_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup PWRROM_15XX CHIP: LPC15xx Power ROM API declarations and functions
 * @ingroup ROMAPI_15XX
 * @{
 */

/**
 * @brief LPC15XX Power ROM APIs - set_pll mode options
 */
#define CPU_FREQ_EQU    0
#define CPU_FREQ_LTE    1
#define CPU_FREQ_GTE    2
#define CPU_FREQ_APPROX 3

/**
 * @brief LPC15XX Power ROM APIs - set_pll response0 options
 */
#define PLL_CMD_SUCCESS    0
#define PLL_INVALID_FREQ   1
#define PLL_INVALID_MODE   2
#define PLL_FREQ_NOT_FOUND 3
#define PLL_NOT_LOCKED     4

/**
 * @brief LPC15XX Power ROM APIs - set_power mode options
 */
#define PWR_DEFAULT         0
#define PWR_CPU_PERFORMANCE 1
#define PWR_EFFICIENCY      2
#define PWR_LOW_CURRENT     3

/**
 * @brief LPC15XX Power ROM APIs - set_power response0 options
 */
#define PWR_CMD_SUCCESS  0
#define PWR_INVALID_FREQ 1
#define PWR_INVALID_MODE 2

/**
 * @brief LPC15XX Power ROM APIs - power_mode_configure mode options
 */
#define PMU_SLEEP           0
#define PMU_DEEP_SLEEP      1
#define PMU_POWERDOWN       2
#define PMU_DEEP_POWERDOWN  3

/**
 * @brief LPC15XX Power ROM APIs - power_mode_configure peripheral control bits
 */
#define PMU_PD_WDOSC         (1 << 0)
#define PMU_PD_BOD           (1 << 1)
#define PMU_PD_ACMP0         (1 << 2)
#define PMU_PD_ACMP1         (1 << 3)
#define PMU_PD_ACMP2         (1 << 4)
#define PMU_PD_ACMP3         (1 << 5)
#define PMU_PD_IREF          (1 << 6)
#define PMU_PD_TS            (1 << 7)

/**
 * @brief LPC15xx Power ROM API structure
 * The power profile API provides functions to configure the system clock and optimize the
 * system setting for lowest power consumption.
 */
typedef struct PWRD_API {
	void (*set_pll)(uint32_t cmd[], uint32_t resp[]);	/*!< Set PLL function */
	void (*set_power)(uint32_t cmd[], uint32_t resp[]);	/*!< Set power function */
	void (*power_mode_configure)(uint32_t power_mode, uint32_t peripheral_ctrl);/*!< Sets the chip is low power modes */
	void (*set_aclkgate)(uint32_t aclkgate);
	uint32_t (*get_aclkgate)(void);
} PWRD_API_T;

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __ROM_PWR_15XX_H_ */
