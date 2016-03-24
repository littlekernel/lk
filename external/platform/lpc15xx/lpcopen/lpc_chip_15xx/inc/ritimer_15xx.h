/*
 * @brief LPC15xx RITimer driver
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

#ifndef __RITIMER_15XX_H_
#define __RITIMER_15XX_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup RIT_15XX CHIP: LPC15xx Repetitive Interrupt Timer driver
 * @ingroup CHIP_15XX_Drivers
 * @{
 */

/**
 * @brief Repetitive Interrupt Timer register block structure
 */
typedef struct {				/*!< RITIMER Structure      */
	__IO uint32_t  COMPVAL;		/*!< Compare register       */
	__IO uint32_t  MASK;		/*!< Mask register. This register holds the 32-bit mask value. A 1 written to any bit will force a compare on the corresponding bit of the counter and compare register. */
	__IO uint32_t  CTRL;		/*!< Control register       */
	__IO uint32_t  COUNTER;		/*!< 32-bit counter         */
	__IO uint32_t  COMPVAL_H;	/*!< Compare upper register */
	__IO uint32_t  MASK_H;		/*!< Mask upper register    */
	__I  uint32_t  RESERVED0[1];
	__IO uint32_t  COUNTER_H;	/*!< Counter upper register */
} LPC_RITIMER_T;

/*
 * RIT control register
 */
/**	Set by H/W when the counter value equals the masked compare value */
#define RIT_CTRL_INT    (1 << 0)
/** Set timer enable clear to 0 when the counter value equals the masked compare value  */
#define RIT_CTRL_ENCLR  (1 << 1)
/** Set timer enable on debug */
#define RIT_CTRL_ENBR   (1 << 2)
/** Set timer enable */
#define RIT_CTRL_TEN    (1 << 3)

/**
 * @brief	Initialize the RIT
 * @param	pRITimer	: RITimer peripheral selected
 * @return	None
 * @note	The timer will not br running after this call. Use
 *			Chip_RIT_Enable() to start the timer running.
 */
void Chip_RIT_Init(LPC_RITIMER_T *pRITimer);

/**
 * @brief	Shutdown the RIT
 * @param	pRITimer	: RITimer peripheral selected
 * @return	None
 */
void Chip_RIT_DeInit(LPC_RITIMER_T *pRITimer);

/**
 * @brief	Safely sets CTRL register bits
 * @param	pRITimer	: RITimer peripheral selected
 * @param	val			: RIT bits to be set, one or more RIT_CTRL_* values
 * @return	None
 */
void Chip_RIT_SetCTRL(LPC_RITIMER_T *pRITimer, uint32_t val);

/**
 * @brief	Safely clears CTRL register bits
 * @param	pRITimer	: RITimer peripheral selected
 * @param	val			: RIT bits to be cleared, one or more RIT_CTRL_* values
 * @return	None
 */
void Chip_RIT_ClearCTRL(LPC_RITIMER_T *pRITimer, uint32_t val);

/**
 * @brief	Enable Timer
 * @param	pRITimer		: RITimer peripheral selected
 * @return	None
 */
STATIC INLINE void Chip_RIT_Enable(LPC_RITIMER_T *pRITimer)
{
	Chip_RIT_SetCTRL(pRITimer, RIT_CTRL_TEN);
}

/**
 * @brief	Disable Timer
 * @param	pRITimer	: RITimer peripheral selected
 * @return	None
 */
STATIC INLINE void Chip_RIT_Disable(LPC_RITIMER_T *pRITimer)
{
	Chip_RIT_ClearCTRL(pRITimer, RIT_CTRL_TEN);
}

/**
 * @brief	Enable timer debug mode
 * @param	pRITimer	: RITimer peripheral selected
 * @return	None
 * @note	This function halts the repetitive timer when
 *			the processor is halted for debugging.
 */
STATIC INLINE void Chip_RIT_EnableDebug(LPC_RITIMER_T *pRITimer)
{
	Chip_RIT_SetCTRL(pRITimer, RIT_CTRL_ENBR);
}

/**
 * @brief	Disable timer debug mode
 * @param	pRITimer	: RITimer peripheral selected
 * @return	None
 * @note	This function allows the repetitive timer to continue running
 *			when the processor is halted for debugging.
 */
STATIC INLINE void Chip_RIT_DisableDebug(LPC_RITIMER_T *pRITimer)
{
	Chip_RIT_ClearCTRL(pRITimer, RIT_CTRL_ENBR);
}

/**
 * @brief	Enables automatic counter clear on compare
 * @param	pRITimer	: RITimer peripheral selected
 * @return	None
 */
STATIC INLINE void Chip_RIT_EnableCompClear(LPC_RITIMER_T *pRITimer)
{
	Chip_RIT_SetCTRL(pRITimer, RIT_CTRL_ENCLR);
}

/**
 * @brief	Disables automatic counter clear on compare
 * @param	pRITimer	: RITimer peripheral selected
 * @return	None
 */
STATIC INLINE void Chip_RIT_DisableCompClear(LPC_RITIMER_T *pRITimer)
{
	Chip_RIT_ClearCTRL(pRITimer, RIT_CTRL_ENCLR);
}

/**
 * @brief	Check whether timer interrupt is pending
 * @param	pRITimer	: RITimer peripheral selected
 * @return	true if the interrupt is pending, otherwise false
 */
STATIC INLINE bool Chip_RIT_GetIntStatus(LPC_RITIMER_T *pRITimer)
{
	return (bool) ((pRITimer->CTRL & RIT_CTRL_INT) != 0);
}

/**
 * @brief	Clears the timer interrupt pending state
 * @param	pRITimer	: RITimer peripheral selected
 * @return	None
 */
STATIC INLINE void Chip_RIT_ClearIntStatus(LPC_RITIMER_T *pRITimer)
{
	Chip_RIT_SetCTRL(pRITimer, RIT_CTRL_INT);
}

/**
 * @brief	Set a tick value for the interrupt to time out
 * @param	pRITimer	: RITimer peripheral selected
 * @param	val			: value (in ticks) for the coounter compare value
 * @return	None
 * @note	The base timer tick rate can be determined by calling
 *			Chip_Clock_GetSystemClockRate().
 */
void Chip_RIT_SetCompareValue(LPC_RITIMER_T *pRITimer, uint64_t val);

/**
 * @brief	Returns the current timer compare value
 * @param	pRITimer	: RITimer peripheral selected
 * @return	the current timer compare value
 */
uint64_t Chip_RIT_GetCompareValue(LPC_RITIMER_T *pRITimer);

/**
 * @brief	Sets a mask value used for bit based compare
 * @param	pRITimer	: RITimer peripheral selected
 * @param	mask		: Mask value for timer (see user manual)
 * @return	None
 */
void Chip_RIT_SetMaskValue(LPC_RITIMER_T *pRITimer, uint64_t mask);

/**
 * @brief	Returns the mask value used for bit based compare
 * @param	pRITimer	: RITimer peripheral selected
 * @return	the current mask value
 */
uint64_t Chip_RIT_GetMaskValue(LPC_RITIMER_T *pRITimer);

/**
 * @brief	Sets the current timer Counter value
 * @param	pRITimer	: RITimer peripheral selected
 * @param	count		: Count value to set timer to
 * @return	Nothing
 */
void Chip_RIT_SetCounter(LPC_RITIMER_T *pRITimer, uint64_t count);

/**
 * @brief	Returns the current timer Counter value
 * @param	pRITimer	: RITimer peripheral selected
 * @return	the current timer counter value
 */
uint64_t Chip_RIT_GetCounter(LPC_RITIMER_T *pRITimer);

/**
 * @brief	Set timer interval value in Hz (frequency)
 * @param	pRITimer	: RITimer peripheral selected
 * @param	freq		: timer interval value in Hz
 * @return	None
 * @note	Will not alter current counter value. Accuracy depends on
 *			base clock rate. Timer enable/disable state is not changed.
 */
void Chip_RIT_SetTimerIntervalHz(LPC_RITIMER_T *pRITimer, uint32_t freq);

/**
 * @brief	Returns base clock rate for timer
 * @param	pRITimer	: RITimer peripheral selected
 * @return	Value in Hz the timer is running at
 * @note	This returned value contains the base clock the timer uses.
 *			If you set the tick count to this return value with the
 *			Chip_RIT_SetCompareValue() function, you will get a 1Hz
 *			interval rate.
 */
STATIC INLINE uint32_t Chip_RIT_GetBaseClock(LPC_RITIMER_T *pRITimer)
{
	return Chip_Clock_GetSystemClockRate();
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __RITIMER_15XX_H_ */
