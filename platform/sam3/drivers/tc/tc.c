/**
 * \file
 *
 * \brief Timer Counter (TC) driver for SAM.
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

#include <assert.h>
#include "tc.h"

/// @cond 0
/**INDENT-OFF**/
#ifdef __cplusplus
extern "C" {
#endif
/**INDENT-ON**/
/// @endcond

/**
 * \defgroup sam_drivers_tc_group Timer Counter (TC)
 *
 * The Timer Counter (TC) includes three identical 32-bit Timer Counter channels.
 * Each channel can be independently programmed to perform a wide range of functions
 * including frequency measurement, event counting, interval measurement, pulse
 * generation, delay timing and pulse width modulation.
 *
 * @{
 */

/**
 * \brief Configure TC for timer, waveform generation or capture.
 *
 * \param p_tc Pointer to a TC instance.
 * \param ul_channel Channel to configure.
 * \param ul_mode Control mode register value to set.
 */
void tc_init(Tc * p_tc, uint32_t ul_channel, uint32_t ul_mode)
{
	TcChannel *tc_channel;

	assert(ul_channel <
			(sizeof(p_tc->TC_CHANNEL) / sizeof(p_tc->TC_CHANNEL[0])));
	tc_channel = p_tc->TC_CHANNEL + ul_channel;

	/*  Disable TC clock. */
	tc_channel->TC_CCR = TC_CCR_CLKDIS;

	/*  Disable interrupts. */
	tc_channel->TC_IDR = 0xFFFFFFFF;

	/*  Clear status register. */
	tc_channel->TC_SR;

	/*  Set mode. */
	tc_channel->TC_CMR = ul_mode;
}

/**
 * \brief Configure TC for Quadrature Decoder Logic.
 * \note tc_init() must be called first.
 *
 * \param p_tc Pointer to a TC instance.
 * \param ul_blockmode Block mode register value to set.
 *
 * \return 0 for OK.
 */
uint32_t tc_init_quad_dec(Tc * p_tc, uint32_t ul_blockmode)
{
	p_tc->TC_BMR = ul_blockmode;
	return 0;
}

#if (SAM3S || SAM3N || SAM3XA || SAM4S)
/**
 * \brief Configure TC for 2-bit Gray Counter for Stepper Motor.
 * \note tc_init() must be called first.
 *
 * \param p_tc Pointer to a TC instance.
 * \param ul_channel Channel to configure.
 * \param ul_steppermode Stepper motor mode register value to set.
 *
 * \return 0 for OK.
 */
uint32_t tc_init_2bit_gray(Tc * p_tc, uint32_t ul_channel,
		uint32_t ul_steppermode)
{
	assert(ul_channel <
			(sizeof(p_tc->TC_CHANNEL) / sizeof(p_tc->TC_CHANNEL[0])));

	p_tc->TC_CHANNEL[ul_channel].TC_SMMR = ul_steppermode;
	return 0;
}
#endif
/**
 * \brief Start TC clock counter on the selected channel.
 *
 * \param p_tc Pointer to a TC instance.
 * \param ul_channel Channel to configure.
 */
void tc_start(Tc * p_tc, uint32_t ul_channel)
{
	assert(ul_channel <
			(sizeof(p_tc->TC_CHANNEL) / sizeof(p_tc->TC_CHANNEL[0])));

	p_tc->TC_CHANNEL[ul_channel].TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG;
}

/**
 * \brief Stop TC clock counter on the selected channel.
 *
 * \param p_tc Pointer to a TC instance.
 * \param ul_channel Channel to configure.
 */
void tc_stop(Tc * p_tc, uint32_t ul_channel)
{
	assert(ul_channel <
			(sizeof(p_tc->TC_CHANNEL) / sizeof(p_tc->TC_CHANNEL[0])));

	p_tc->TC_CHANNEL[ul_channel].TC_CCR = TC_CCR_CLKDIS;
}

/**
 * \brief Read RA TC counter on the selected channel.
 *
 * \param p_tc Pointer to a TC instance.
 * \param ul_channel Channel to configure.
 *
 * \return RA value.
 */
int tc_read_ra(Tc * p_tc, uint32_t ul_channel)
{
	assert(ul_channel <
			(sizeof(p_tc->TC_CHANNEL) / sizeof(p_tc->TC_CHANNEL[0])));

	return p_tc->TC_CHANNEL[ul_channel].TC_RA;
}

/**
 * \brief Read RB TC counter on the selected channel.
 *
 * \param p_tc Pointer to a TC instance.
 * \param ul_channel Channel to configure.
 *
 * \return RB value.
 */
int tc_read_rb(Tc * p_tc, uint32_t ul_channel)
{
	assert(ul_channel <
			(sizeof(p_tc->TC_CHANNEL) / sizeof(p_tc->TC_CHANNEL[0])));

	return p_tc->TC_CHANNEL[ul_channel].TC_RB;
}

/**
 * \brief Read RC TC counter on the selected channel.
 *
 * \param p_tc Pointer to a TC instance.
 * \param ul_channel Channel to configure.
 *
 * \return RC value.
 */
int tc_read_rc(Tc * p_tc, uint32_t ul_channel)
{
	assert(ul_channel <
			(sizeof(p_tc->TC_CHANNEL) / sizeof(p_tc->TC_CHANNEL[0])));

	return p_tc->TC_CHANNEL[ul_channel].TC_RC;
}

/**
 * \brief Write RA TC counter on the selected channel.
 *
 * \param p_tc Pointer to a TC instance.
 * \param ul_channel Channel to configure.
 * \param ul_value Value to set in register.
 */
void tc_write_ra(Tc * p_tc, uint32_t ul_channel,
		uint32_t ul_value)
{
	assert(ul_channel <
			(sizeof(p_tc->TC_CHANNEL) / sizeof(p_tc->TC_CHANNEL[0])));

	p_tc->TC_CHANNEL[ul_channel].TC_RA = ul_value;
}

/**
 * \brief Write RB TC counter on the selected channel.
 *
 * \param p_tc Pointer to a TC instance.
 * \param ul_channel Channel to configure.
 * \param ul_value Value to set in register.
 */
void tc_write_rb(Tc * p_tc, uint32_t ul_channel,
		uint32_t ul_value)
{
	assert(ul_channel <
			(sizeof(p_tc->TC_CHANNEL) / sizeof(p_tc->TC_CHANNEL[0])));

	p_tc->TC_CHANNEL[ul_channel].TC_RB = ul_value;
}

/**
 * \brief Write RC TC counter on the selected channel.
 *
 * \param p_tc Pointer to a TC instance.
 * \param ul_channel Channel to configure.
 * \param ul_value Value to set in register.
 */
void tc_write_rc(Tc * p_tc, uint32_t ul_channel,
		uint32_t ul_value)
{
	assert(ul_channel <
			(sizeof(p_tc->TC_CHANNEL) / sizeof(p_tc->TC_CHANNEL[0])));

	p_tc->TC_CHANNEL[ul_channel].TC_RC = ul_value;
}

/**
 * \brief Enable TC interrupts on the selected channel.
 *
 * \param p_tc Pointer to a TC instance.
 * \param ul_channel Channel to configure.
 * \param ul_sources Interrupt sources bit map.
 */
void tc_enable_interrupt(Tc * p_tc, uint32_t ul_channel,
		uint32_t ul_sources)
{
	TcChannel *tc_channel;

	assert(ul_channel <
			(sizeof(p_tc->TC_CHANNEL) / sizeof(p_tc->TC_CHANNEL[0])));
	tc_channel = p_tc->TC_CHANNEL + ul_channel;
	tc_channel->TC_IER = ul_sources;
}

/**
 * \brief Disable TC interrupts on the selected channel.
 *
 * \param p_tc Pointer to a TC instance.
 * \param ul_channel Channel to configure.
 * \param ul_sources Interrupt sources bit map.
 */
void tc_disable_interrupt(Tc * p_tc, uint32_t ul_channel,
		uint32_t ul_sources)
{
	TcChannel *tc_channel;

	assert(ul_channel <
			(sizeof(p_tc->TC_CHANNEL) / sizeof(p_tc->TC_CHANNEL[0])));
	tc_channel = p_tc->TC_CHANNEL + ul_channel;
	tc_channel->TC_IDR = ul_sources;
}

/**
 * \brief Read TC interrupt mask on the selected channel.
 *
 * \param p_tc Pointer to a TC instance.
 * \param ul_channel Channel to configure.
 *
 * \return The interrupt mask value.
 */
uint32_t tc_get_interrupt_mask(Tc * p_tc, uint32_t ul_channel)
{
	TcChannel *tc_channel;

	assert(ul_channel <
			(sizeof(p_tc->TC_CHANNEL) / sizeof(p_tc->TC_CHANNEL[0])));
	tc_channel = p_tc->TC_CHANNEL + ul_channel;
	return tc_channel->TC_IMR;
}

/**
 * \brief Get current status on the selected channel.
 *
 * \param p_tc Pointer to a TC instance.
 * \param ul_channel Channel to configure.
 *
 * \return The current TC status.
 */
uint32_t tc_get_status(Tc * p_tc, uint32_t ul_channel)
{
	TcChannel *tc_channel;

	assert(ul_channel <
			(sizeof(p_tc->TC_CHANNEL) / sizeof(p_tc->TC_CHANNEL[0])));
	tc_channel = p_tc->TC_CHANNEL + ul_channel;
	return tc_channel->TC_SR;
}

uint32_t tc_get_cv(Tc * p_tc, uint32_t ul_channel)
{
	TcChannel *tc_channel;

	assert(ul_channel <
			(sizeof(p_tc->TC_CHANNEL) / sizeof(p_tc->TC_CHANNEL[0])));
	tc_channel = p_tc->TC_CHANNEL + ul_channel;
	return tc_channel->TC_CV;
}

#ifndef FREQ_SLOW_CLOCK_EXT
#define FREQ_SLOW_CLOCK_EXT 32768 /* External slow clock frequency (hz) */
#endif
#define TC_DIV_FACTOR 65536 /* TC divisor used to find the lowest acceptable timer frequency */

/**
 * \brief Find the best MCK divisor.
 *
 * Finds the best MCK divisor given the timer frequency and MCK. The result
 * is guaranteed to satisfy the following equation:
 * \code
 *   (MCK / (DIV * 65536)) <= freq <= (MCK / DIV)
 * \endcode
 * with DIV being the lowest possible value,
 * to maximize timing adjust resolution.
 *
 * \param ul_freq  Desired timer frequency.
 * \param ul_mck  Master clock frequency.
 * \param p_uldiv  Divisor value.
 * \param p_ultcclks  TCCLKS field value for divisor.
 * \param ul_boardmck  Board clock frequency.
 *
 * \return 1 if a proper divisor has been found, otherwise 0.
 */
uint32_t tc_find_mck_divisor(uint32_t ul_freq, uint32_t ul_mck,
		uint32_t * p_uldiv, uint32_t * p_ultcclks, uint32_t ul_boardmck)
{
	const uint32_t divisors[5] = { 2, 8, 32, 128,
			ul_boardmck / FREQ_SLOW_CLOCK_EXT };
	uint32_t ul_index;
	uint32_t ul_high, ul_low;

	/*  Satisfy frequency bound. */
	for (ul_index = 0;
			ul_index < (sizeof(divisors) / sizeof(divisors[0]));
			ul_index ++) {
		ul_high = ul_mck / divisors[ul_index];
		ul_low  = ul_high / TC_DIV_FACTOR;
		if (ul_freq > ul_high) {
			return 0;
		} else if (ul_freq >= ul_low) {
			break;
		}
	}
	if (ul_index >= (sizeof(divisors) / sizeof(divisors[0]))) {
		return 0;
	}

	/*  Store results. */
	if (p_uldiv) {
		*p_uldiv = divisors[ul_index];
	}
	if (p_ultcclks) {
		*p_ultcclks = ul_index;
	}

	return 1;
}

/**
 * \brief Enable TC QDEC interrupts.
 *
 * \param p_tc Pointer to a TC instance.
 * \param ul_sources Interrupts to be enabled.
 */
void tc_enable_qdec_interrupt(Tc * p_tc, uint32_t ul_sources)
{
	p_tc->TC_QIER = ul_sources;
}

/**
 * \brief Disable TC QDEC interrupts.
 *
 * \param p_tc Pointer to a TC instance.
 * \param ul_sources Interrupts to be disabled.
 */
void tc_disable_qdec_interrupt(Tc * p_tc, uint32_t ul_sources)
{
	p_tc->TC_QIDR = ul_sources;
}

/**
 * \brief Read TC QDEC interrupt mask.
 *
 * \param p_tc Pointer to a TC instance.
 *
 * \return The interrupt mask value.
 */
uint32_t tc_get_qdec_interrupt_mask(Tc * p_tc)
{
	return p_tc->TC_QIMR;
}

/**
 * \brief Get current QDEC status.
 *
 * \param p_tc Pointer to a TC instance.
 *
 * \return The current TC status.
 */
uint32_t tc_get_qdec_interrupt_status(Tc * p_tc)
{
	return p_tc->TC_QISR;
}

#if (SAM3S || SAM3N || SAM3XA || SAM4S)
/**
 * \brief Enable or disable write protection of TC registers.
 *
 * \param p_tc Pointer to a TC instance.
 * \param ul_enable 1 to enable, 0 to disable.
 */
void tc_set_writeprotect(Tc * p_tc, uint32_t ul_enable)
{
	p_tc->TC_WPMR |= ul_enable;
}
#endif

//@}

/// @cond 0
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/// @endcond
