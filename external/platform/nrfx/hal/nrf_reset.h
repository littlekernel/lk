/*
 * Copyright (c) 2019 - 2020, Nordic Semiconductor ASA
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

#ifndef NRF_RESET_H__
#define NRF_RESET_H__

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(NRF5340_XXAA_NETWORK) || defined(__NRFX_DOXYGEN__)
/** @brief Presence of Network core RESET functionality. */
#define NRF_RESET_HAS_NETWORK 1
#else
#define NRF_RESET_HAS_NETWORK 0
#endif

#if defined(NRF5340_XXAA_APPLICATION) || defined(__NRFX_DOXYGEN__)
/** @brief Presence of Application core RESET functionality. */
#define NRF_RESET_HAS_APPLICATION 1
#else
#define NRF_RESET_HAS_APPLICATION 0
#endif

/**
 * @defgroup nrf_reset_hal RESET HAL
 * @{
 * @ingroup nrf_reset
 * @brief   Hardware access layer for managing the RESET peripheral.
 */

/** @brief Reset reason bit masks. */
typedef enum
{
    NRF_RESET_RESETREAS_RESETPIN_MASK  = RESET_RESETREAS_RESETPIN_Msk,  ///< Bit mask of RESETPIN field.
    NRF_RESET_RESETREAS_DOG0_MASK      = RESET_RESETREAS_DOG0_Msk,      ///< Bit mask of DOG0 field.
    NRF_RESET_RESETREAS_CTRLAP_MASK    = RESET_RESETREAS_CTRLAP_Msk,    ///< Bit mask of CTRLAP field.
    NRF_RESET_RESETREAS_SREQ_MASK      = RESET_RESETREAS_SREQ_Msk,      ///< Bit mask of SREQ field.
    NRF_RESET_RESETREAS_LOCKUP_MASK    = RESET_RESETREAS_LOCKUP_Msk,    ///< Bit mask of LOCKUP field.
    NRF_RESET_RESETREAS_OFF_MASK       = RESET_RESETREAS_OFF_Msk,       ///< Bit mask of OFF field.
    NRF_RESET_RESETREAS_LPCOMP_MASK    = RESET_RESETREAS_LPCOMP_Msk,    ///< Bit mask of LPCOMP field.
    NRF_RESET_RESETREAS_DIF_MASK       = RESET_RESETREAS_DIF_Msk,       ///< Bit mask of DIF field.
#if NRF_RESET_HAS_NETWORK
    NRF_RESET_RESETREAS_LSREQ_MASK     = RESET_RESETREAS_LSREQ_Msk,     ///< Bit mask of LSREQ field.
    NRF_RESET_RESETREAS_LLOCKUP_MASK   = RESET_RESETREAS_LLOCKUP_Msk,   ///< Bit mask of LLOCKUP field.
    NRF_RESET_RESETREAS_LDOG_MASK      = RESET_RESETREAS_LDOG_Msk,      ///< Bit mask of LDOG field.
    NRF_RESET_RESETREAS_MFORCEOFF_MASK = RESET_RESETREAS_MFORCEOFF_Msk, ///< Bit mask of MFORCEOFF field.
#endif
    NRF_RESET_RESETREAS_NFC_MASK       = RESET_RESETREAS_NFC_Msk,       ///< Bit mask of NFC field.
    NRF_RESET_RESETREAS_DOG1_MASK      = RESET_RESETREAS_DOG1_Msk,      ///< Bit mask of DOG1 field.
    NRF_RESET_RESETREAS_VBUS_MASK      = RESET_RESETREAS_VBUS_Msk,      ///< Bit mask of VBUS field.
#if NRF_RESET_HAS_NETWORK
    NRF_RESET_RESETREAS_LCTRLAP_MASK   = RESET_RESETREAS_LCTRLAP_Msk,   ///< Bit mask of LCTRLAP field.
#endif
} nrf_reset_resetreas_mask_t;

/**
 * @brief Function for getting the reset reason bitmask.
 *
 * This function returns the reset reason bitmask.
 * Unless cleared, the RESETREAS register is cumulative.
 * If none of the reset sources is flagged, the chip was reset from the on-chip reset generator.
 * This indicates a power-on-reset or a brown out reset.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return The mask of reset reasons constructed with @ref nrf_reset_resetreas_mask_t.
 */
NRF_STATIC_INLINE uint32_t nrf_reset_resetreas_get(NRF_RESET_Type const * p_reg);

/**
 * @brief Function for clearing the selected reset reason field.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  The mask constructed from @ref nrf_reset_resetreas_mask_t enumerator values.
 */
NRF_STATIC_INLINE void nrf_reset_resetreas_clear(NRF_RESET_Type * p_reg, uint32_t mask);

#if NRF_RESET_HAS_APPLICATION
/**
 * @brief Function for setting the force off signal for the Network core.
 *
 * A force off will reset the Network core and switch off its power and clocks.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] hold  True if the force off signal is to be held.
 *                  False if the force off signal is to be released.
 */
NRF_STATIC_INLINE void nrf_reset_network_force_off(NRF_RESET_Type * p_reg, bool hold);
#endif // NRF_RESET_HAS_APPLICATION

#ifndef NRF_DECLARE_ONLY

NRF_STATIC_INLINE uint32_t nrf_reset_resetreas_get(NRF_RESET_Type const * p_reg)
{
    return p_reg->RESETREAS;
}

NRF_STATIC_INLINE void nrf_reset_resetreas_clear(NRF_RESET_Type * p_reg, uint32_t mask)
{
    p_reg->RESETREAS = mask;
}

#if NRF_RESET_HAS_APPLICATION
NRF_STATIC_INLINE void nrf_reset_network_force_off(NRF_RESET_Type * p_reg, bool hold)
{
    p_reg->NETWORK.FORCEOFF = (hold ? RESET_NETWORK_FORCEOFF_FORCEOFF_Hold :
                                      RESET_NETWORK_FORCEOFF_FORCEOFF_Release);
}
#endif // NRF_RESET_HAS_APPLICATION

#endif // NRF_DECLARE_ONLY

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRF_RESET_H__
