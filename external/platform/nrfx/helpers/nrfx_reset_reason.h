/*
 * Copyright (c) 2020, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef NRFX_RESET_REASON_H
#define NRFX_RESET_REASON_H

#include <nrfx.h>
#include <hal/nrf_power.h>

#if !NRF_POWER_HAS_RESETREAS
#include <hal/nrf_reset.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrfx_reset_reason Generic Reset Reason layer
 * @{
 * @ingroup nrfx
 * @ingroup nrf_power
 * @ingroup nrf_reset
 *
 * @brief Helper layer that provides a uniform way of checking the reset reason.
 */

/** @brief Reset reason bit masks. */
typedef enum
{
#if !NRF_POWER_HAS_RESETREAS || defined(__NRFX_DOXYGEN__)
    NRFX_RESET_REASON_RESETPIN_MASK  = RESET_RESETREAS_RESETPIN_Msk,
    /**< Reset from pin-reset detected. */
    NRFX_RESET_REASON_DOG0_MASK      = RESET_RESETREAS_DOG0_Msk,
    /**< Reset from watchdog/application watchdong timer 0 detected. */
    NRFX_RESET_REASON_DOG_MASK       = NRFX_RESET_REASON_DOG0_MASK,
    /**< Reset from watchdog/application watchdong timer 0 detected. */
    NRFX_RESET_REASON_CTRLAP_MASK    = RESET_RESETREAS_CTRLAP_Msk,
    /**< Reset from application CTRL-AP detected. */
    NRFX_RESETREAS_SREQ_MASK         = RESET_RESETREAS_SREQ_Msk,
    /**< Reset from soft reset/application soft reset detected. */
    NRFX_RESET_REASON_LOCKUP_MASK    = RESET_RESETREAS_LOCKUP_Msk,
    /**< Reset from CPU lockup/application CPU lockup detected. */
    NRFX_RESET_REASON_OFF_MASK       = RESET_RESETREAS_OFF_Msk,
    /**< Reset due to wakeup from System OFF mode when wakeup is triggered by DETECT signal from
     *   GPIO. */
    NRFX_RESET_REASON_LPCOMP_MASK    = RESET_RESETREAS_LPCOMP_Msk,
    /**< Reset due to wakeup from System OFF mode when wakeup is triggered by ANADETECT signal from
     *   LPCOMP. */
    NRFX_RESET_REASON_DIF_MASK       = RESET_RESETREAS_DIF_Msk,
    /**< Reset due to wakeup from System OFF mode when wakeup is triggered by entering the debug
     *   interface mode. */
#if NRF_RESET_HAS_NETWORK
    NRFX_RESET_REASON_LSREQ_MASK     = RESET_RESETREAS_LSREQ_Msk,
    /**< Reset from network soft reset detected. */
    NRFX_RESET_REASON_LLOCKUP_MASK   = RESET_RESETREAS_LLOCKUP_Msk,
    /**< Reset from network CPU lockup detected. */
    NRFX_RESET_REASON_LDOG_MASK      = RESET_RESETREAS_LDOG_Msk,
    /**< Reset from network watchdog timer detected. */
    NRFX_RESET_REASON_MFORCEOFF_MASK = RESET_RESETREAS_MFORCEOFF_Msk,
    /**< Force off reset from application core detected. */
#endif // NRF_RESET_HAS_NETWORK
    NRFX_RESET_REASON_NFC_MASK       = RESET_RESETREAS_NFC_Msk,
    /**< Reset after wakeup from System OFF mode due to NRF field being detected. */
    NRFX_RESET_REASON_DOG1_MASK      = RESET_RESETREAS_DOG1_Msk,
    /**< Reset from application watchdog timer 1 detected. */
    NRFX_RESET_REASON_VBUS_MASK      = RESET_RESETREAS_VBUS_Msk,
    /**< Reset after wakeup from System OFF mode due to VBUS rising into valid range. */
#if NRF_RESET_HAS_NETWORK
    NRFX_RESET_REASON_LCTRLAP_MASK   = RESET_RESETREAS_LCTRLAP_Msk,
    /**< Reset from network CTRL-AP detected. */
#endif // NRF_RESET_HAS_NETWORK
#else
    NRFX_RESET_REASON_RESETPIN_MASK  = POWER_RESETREAS_RESETPIN_Msk,
    NRFX_RESET_REASON_DOG_MASK       = POWER_RESETREAS_DOG_Msk,
    NRFX_RESET_REASON_SREQ_MASK      = POWER_RESETREAS_SREQ_Msk ,
    NRFX_RESET_REASON_LOCKUP_MASK    = POWER_RESETREAS_LOCKUP_Msk,
    NRFX_RESET_REASON_OFF_MASK       = POWER_RESETREAS_OFF_Msk,
#if defined(POWER_RESETREAS_LPCOMP_Msk)
    NRFX_RESET_REASON_LPCOMP_MASK    = POWER_RESETREAS_LPCOMP_Msk,
#endif
    NRFX_RESET_REASON_DIF_MASK       = POWER_RESETREAS_DIF_Msk,
#if defined(POWER_RESETREAS_NFC_Msk)
    NRFX_RESET_REASON_NFC_MASK       = POWER_RESETREAS_NFC_Msk,
#endif
#if defined(POWER_RESETREAS_VBUS_Msk)
    NRFX_RESET_REASON_VBUS_MASK      = POWER_RESETREAS_VBUS_Msk,
#endif
#endif // !NRF_POWER_HAS_RESETREAS || defined(__NRFX_DOXYGEN__)
} nrfx_reset_reason_mask_t;

/**
 * @brief Function for getting the reset reason bitmask.
 *
 * Unless cleared, the RESETREAS register is cumulative.
 * If none of the reset sources is flagged, the chip was reset from the on-chip reset generator.
 * This indicates a power-on-reset or a brown out reset.
 *
 * @return Mask of reset reasons constructed from @ref nrfx_reset_reason_mask_t values.
 */
__STATIC_INLINE uint32_t nrfx_reset_reason_get(void)
{
#if NRF_POWER_HAS_RESETREAS
    return nrf_power_resetreas_get(NRF_POWER);
#else
    return nrf_reset_resetreas_get(NRF_RESET);
#endif
}

/**
 * @brief Function for clearing the selected reset reason fields.
 *
 * @param[in] mask Mask constructed from @ref nrfx_reset_reason_mask_t values.
 */
__STATIC_INLINE void nrfx_reset_reason_clear(uint32_t mask)
{
#if NRF_POWER_HAS_RESETREAS
    nrf_power_resetreas_clear(NRF_POWER, mask);
#else
    nrf_reset_resetreas_clear(NRF_RESET, mask);
#endif
}

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRFX_RESET_REASON_H
