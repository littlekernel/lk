/*
 * Copyright (c) 2018 - 2020, Nordic Semiconductor ASA
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

#ifndef NRF_REGULATORS_H__
#define NRF_REGULATORS_H__

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_regulators_hal REGULATORS HAL
 * @{
 * @ingroup nrf_power
 * @brief   Hardware access layer for managing the REGULATORS peripheral.
 */

#if defined(REGULATORS_MAINREGSTATUS_VREGH_Msk) || defined(__NRFX_DOXYGEN__)
/** @brief Symbol indicating whether MAINREGSTATUS register is present. */
#define NRF_REGULATORS_HAS_MAINREGSTATUS 1
#else
#define NRF_REGULATORS_HAS_MAINREGSTATUS 0
#endif

#if defined(REGULATORS_POFCON_POF_Msk) || defined(__NRFX_DOXYGEN__)
/** @brief Symbol indicating whether power failure comparator is present. */
#define NRF_REGULATORS_HAS_POFCON 1
#else
#define NRF_REGULATORS_HAS_POFCON 0
#endif

#if defined(REGULATORS_POFCON_THRESHOLDVDDH_Msk) || defined(__NRFX_DOXYGEN__)
/** @brief Symbol indicating whether power failure comparator for VDDH is present. */
#define NRF_REGULATORS_HAS_POFCON_VDDH 1
#else
#define NRF_REGULATORS_HAS_POFCON_VDDH 0
#endif

#if defined(REGULATORS_VREGH_DCDCEN_DCDCEN_Msk) || defined(__NRFX_DOXYGEN__)
/** @brief Symbol indicating whether high voltage regulator is present. */
#define NRF_REGULATORS_HAS_DCDCEN_VDDH 1
#else
#define NRF_REGULATORS_HAS_DCDCEN_VDDH 0
#endif

#if defined(REGULATORS_VREGRADIO_DCDCEN_DCDCEN_Msk) || defined(__NRFX_DOXYGEN__)
/** @brief Symbol indicating whether radio regulator is present. */
#define NRF_REGULATORS_HAS_DCDCEN_RADIO 1
#else
#define NRF_REGULATORS_HAS_DCDCEN_RADIO 0
#endif

#if NRF_REGULATORS_HAS_MAINREGSTATUS
/** @brief Main regulator status. */
typedef enum
{
    NRF_REGULATORS_MAINREGSTATUS_NORMAL = REGULATORS_MAINREGSTATUS_VREGH_Inactive, /**< Normal voltage mode. Voltage supplied on VDD and VDDH. */
    NRF_REGULATORS_MAINREGSTATUS_HIGH   = REGULATORS_MAINREGSTATUS_VREGH_Active    /**< High voltage mode. Voltage supplied on VDDH. */
} nrf_regulators_mainregstatus_t;
#endif

#if NRF_REGULATORS_HAS_POFCON
/** @brief Power failure comparator thresholds. */
typedef enum
{
    NRF_REGULATORS_POFTHR_V19 = REGULATORS_POFCON_THRESHOLD_V19, /**< Set threshold to 1.9&nbsp;V. */
    NRF_REGULATORS_POFTHR_V20 = REGULATORS_POFCON_THRESHOLD_V20, /**< Set threshold to 2.0&nbsp;V. */
    NRF_REGULATORS_POFTHR_V21 = REGULATORS_POFCON_THRESHOLD_V21, /**< Set threshold to 2.1&nbsp;V. */
    NRF_REGULATORS_POFTHR_V22 = REGULATORS_POFCON_THRESHOLD_V22, /**< Set threshold to 2.2&nbsp;V. */
    NRF_REGULATORS_POFTHR_V23 = REGULATORS_POFCON_THRESHOLD_V23, /**< Set threshold to 2.3&nbsp;V. */
    NRF_REGULATORS_POFTHR_V24 = REGULATORS_POFCON_THRESHOLD_V24, /**< Set threshold to 2.4&nbsp;V. */
    NRF_REGULATORS_POFTHR_V25 = REGULATORS_POFCON_THRESHOLD_V25, /**< Set threshold to 2.5&nbsp;V. */
    NRF_REGULATORS_POFTHR_V26 = REGULATORS_POFCON_THRESHOLD_V26, /**< Set threshold to 2.6&nbsp;V. */
    NRF_REGULATORS_POFTHR_V27 = REGULATORS_POFCON_THRESHOLD_V27, /**< Set threshold to 2.7&nbsp;V. */
    NRF_REGULATORS_POFTHR_V28 = REGULATORS_POFCON_THRESHOLD_V28, /**< Set threshold to 2.8&nbsp;V. */
} nrf_regulators_pof_thr_t;
#endif

#if NRF_REGULATORS_HAS_POFCON_VDDH
/** @brief Power failure comparator thresholds for VDDH. */
typedef enum
{
    NRF_REGULATORS_POFTHRVDDH_V27 = REGULATORS_POFCON_THRESHOLDVDDH_V27, /**< Set threshold to 2.7&nbsp;V. */
    NRF_REGULATORS_POFTHRVDDH_V28 = REGULATORS_POFCON_THRESHOLDVDDH_V28, /**< Set threshold to 2.8&nbsp;V. */
    NRF_REGULATORS_POFTHRVDDH_V29 = REGULATORS_POFCON_THRESHOLDVDDH_V29, /**< Set threshold to 2.9&nbsp;V. */
    NRF_REGULATORS_POFTHRVDDH_V30 = REGULATORS_POFCON_THRESHOLDVDDH_V30, /**< Set threshold to 3.0&nbsp;V. */
    NRF_REGULATORS_POFTHRVDDH_V31 = REGULATORS_POFCON_THRESHOLDVDDH_V31, /**< Set threshold to 3.1&nbsp;V. */
    NRF_REGULATORS_POFTHRVDDH_V32 = REGULATORS_POFCON_THRESHOLDVDDH_V32, /**< Set threshold to 3.2&nbsp;V. */
    NRF_REGULATORS_POFTHRVDDH_V33 = REGULATORS_POFCON_THRESHOLDVDDH_V33, /**< Set threshold to 3.3&nbsp;V. */
    NRF_REGULATORS_POFTHRVDDH_V34 = REGULATORS_POFCON_THRESHOLDVDDH_V34, /**< Set threshold to 3.4&nbsp;V. */
    NRF_REGULATORS_POFTHRVDDH_V35 = REGULATORS_POFCON_THRESHOLDVDDH_V35, /**< Set threshold to 3.5&nbsp;V. */
    NRF_REGULATORS_POFTHRVDDH_V36 = REGULATORS_POFCON_THRESHOLDVDDH_V36, /**< Set threshold to 3.6&nbsp;V. */
    NRF_REGULATORS_POFTHRVDDH_V37 = REGULATORS_POFCON_THRESHOLDVDDH_V37, /**< Set threshold to 3.7&nbsp;V. */
    NRF_REGULATORS_POFTHRVDDH_V38 = REGULATORS_POFCON_THRESHOLDVDDH_V38, /**< Set threshold to 3.8&nbsp;V. */
    NRF_REGULATORS_POFTHRVDDH_V39 = REGULATORS_POFCON_THRESHOLDVDDH_V39, /**< Set threshold to 3.9&nbsp;V. */
    NRF_REGULATORS_POFTHRVDDH_V40 = REGULATORS_POFCON_THRESHOLDVDDH_V40, /**< Set threshold to 4.0&nbsp;V. */
    NRF_REGULATORS_POFTHRVDDH_V41 = REGULATORS_POFCON_THRESHOLDVDDH_V41, /**< Set threshold to 4.1&nbsp;V. */
    NRF_REGULATORS_POFTHRVDDH_V42 = REGULATORS_POFCON_THRESHOLDVDDH_V42, /**< Set threshold to 4.2&nbsp;V. */
} nrf_regulators_pof_thrvddh_t;
#endif

/**
 * @brief Function for enabling or disabling DCDC converter.
 *
 * @param[in] p_reg  Pointer to the structure of registers of the peripheral.
 * @param[in] enable True if DCDC converter is to be enabled, false otherwise.
 */
NRF_STATIC_INLINE void nrf_regulators_dcdcen_set(NRF_REGULATORS_Type * p_reg, bool enable);

/**
 * @brief Function for putting CPU in system OFF mode.
 *
 * This function puts the CPU into system off mode.
 * The only way to wake up the CPU is by reset.
 *
 * @note This function never returns.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 */
NRF_STATIC_INLINE void nrf_regulators_system_off(NRF_REGULATORS_Type * p_reg);

#if NRF_REGULATORS_HAS_MAINREGSTATUS
/**
 * @brief Function for getting the main supply status.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return The current main supply status.
 */
NRF_STATIC_INLINE
nrf_regulators_mainregstatus_t nrf_regulators_mainregstatus_get(NRF_REGULATORS_Type const * p_reg);
#endif

#if NRF_REGULATORS_HAS_POFCON
/**
 * @brief Function for setting the power failure comparator configuration.
 *
 * @param[in] p_reg  Pointer to the structure of registers of the peripheral.
 * @param[in] enable True if power failure comparator is to be enabled, false otherwise.
 * @param[in] thr    Voltage threshold value.
 */
NRF_STATIC_INLINE void nrf_regulators_pofcon_set(NRF_REGULATORS_Type *    p_reg,
                                                 bool                     enable,
                                                 nrf_regulators_pof_thr_t thr);

/**
 * @brief Function for getting the power failure comparator configuration.
 *
 * @param[in]  p_reg     Pointer to the structure of registers of the peripheral.
 * @param[out] p_enabled Function sets this boolean variable to true
 *                       if power failure comparator is enabled.
 *                       The pointer can be NULL if we do not need this information.
 *
 * @return Threshold setting for power failure comparator.
 */
NRF_STATIC_INLINE
nrf_regulators_pof_thr_t nrf_regulators_pofcon_get(NRF_REGULATORS_Type const * p_reg,
                                                   bool *                      p_enabled);
#endif // NRF_REGULATORS_HAS_POFCON

#if NRF_REGULATORS_HAS_POFCON_VDDH
/**
 * @brief Function for setting the VDDH power failure comparator threshold.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] thr   Threshold to be set.
 */
NRF_STATIC_INLINE void nrf_regulators_pofcon_vddh_set(NRF_REGULATORS_Type *        p_reg,
                                                      nrf_regulators_pof_thrvddh_t thr);

/**
 * @brief Function for getting the VDDH regulators failure comparator threshold.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return Threshold setting for regulators failure comparator.
 */
NRF_STATIC_INLINE
nrf_regulators_pof_thrvddh_t nrf_regulators_pofcon_vddh_get(NRF_REGULATORS_Type const * p_reg);
#endif // NRF_REGULATORS_HAS_POFCON_VDDH

#if NRF_REGULATORS_HAS_DCDCEN_VDDH
/**
 * @brief Function for enabling or disabling the high voltage regulator.
 *
 * @param[in] p_reg  Pointer to the structure of registers of the peripheral.
 * @param[in] enable True if the high voltage regulator is to be enabled, false otherwise.
 */
NRF_STATIC_INLINE void nrf_regulators_dcdcen_vddh_set(NRF_REGULATORS_Type * p_reg, bool enable);
#endif

#if NRF_REGULATORS_HAS_DCDCEN_RADIO
/**
 * @brief Function for enabling or disabling the radio regulator.
 *
 * @param[in] p_reg  Pointer to the structure of registers of the peripheral.
 * @param[in] enable True if the radio regulator is to be enabled, false otherwise.
 */
NRF_STATIC_INLINE void nrf_regulators_dcdcen_radio_set(NRF_REGULATORS_Type * p_reg, bool enable);
#endif

#ifndef NRF_DECLARE_ONLY

NRF_STATIC_INLINE void nrf_regulators_dcdcen_set(NRF_REGULATORS_Type * p_reg, bool enable)
{
#if defined(REGULATORS_DCDCEN_DCDCEN_Msk)
    p_reg->DCDCEN = (enable ? REGULATORS_DCDCEN_DCDCEN_Msk : 0);
#else
    p_reg->VREGMAIN.DCDCEN = (enable ? REGULATORS_VREGMAIN_DCDCEN_DCDCEN_Msk : 0);
#endif
}

NRF_STATIC_INLINE void nrf_regulators_system_off(NRF_REGULATORS_Type * p_reg)
{
    p_reg->SYSTEMOFF = REGULATORS_SYSTEMOFF_SYSTEMOFF_Msk;
    __DSB();

    /* Solution for simulated System OFF in debug mode */
    while (true)
    {
        __WFE();
    }
}

#if NRF_REGULATORS_HAS_MAINREGSTATUS
NRF_STATIC_INLINE
nrf_regulators_mainregstatus_t nrf_regulators_mainregstatus_get(NRF_REGULATORS_Type const * p_reg)
{
    return (nrf_regulators_mainregstatus_t)p_reg->MAINREGSTATUS;
}
#endif

#if NRF_REGULATORS_HAS_POFCON
NRF_STATIC_INLINE void nrf_regulators_pofcon_set(NRF_REGULATORS_Type *    p_reg,
                                                 bool                     enable,
                                                 nrf_regulators_pof_thr_t thr)
{
#if NRF_REGULATORS_HAS_POFCON_VDDH
    uint32_t pofcon = p_reg->POFCON;
    pofcon &= ~(REGULATORS_POFCON_THRESHOLD_Msk | REGULATORS_POFCON_POF_Msk);
    pofcon |=
#else // NRF_REGULATORS_HAS_POFCON_VDDH
    p_reg->POFCON =
#endif
        (((uint32_t)thr) << REGULATORS_POFCON_THRESHOLD_Pos) |
        (enable ?
        (REGULATORS_POFCON_POF_Enabled << REGULATORS_POFCON_POF_Pos)
        :
        (REGULATORS_POFCON_POF_Disabled << REGULATORS_POFCON_POF_Pos));
#if NRF_REGULATORS_HAS_POFCON_VDDH
    p_reg->POFCON = pofcon;
#endif
}

NRF_STATIC_INLINE
nrf_regulators_pof_thr_t nrf_regulators_pofcon_get(NRF_REGULATORS_Type const * p_reg,
                                                   bool *                      p_enabled)
{
    uint32_t pofcon = p_reg->POFCON;
    if (NULL != p_enabled)
    {
        (*p_enabled) = ((pofcon & REGULATORS_POFCON_POF_Msk) >> REGULATORS_POFCON_POF_Pos)
            == REGULATORS_POFCON_POF_Enabled;
    }
    return (nrf_regulators_pof_thr_t)((pofcon & REGULATORS_POFCON_THRESHOLD_Msk) >>
                                      REGULATORS_POFCON_THRESHOLD_Pos);
}
#endif // NRF_REGULATORS_HAS_POFCON

#if NRF_REGULATORS_HAS_POFCON_VDDH
NRF_STATIC_INLINE void nrf_regulators_pofcon_vddh_set(NRF_REGULATORS_Type *        p_reg,
                                                      nrf_regulators_pof_thrvddh_t thr)
{
    uint32_t pofcon = p_reg->POFCON;
    pofcon &= ~REGULATORS_POFCON_THRESHOLDVDDH_Msk;
    pofcon |= (((uint32_t)thr) << REGULATORS_POFCON_THRESHOLDVDDH_Pos);
    p_reg->POFCON = pofcon;
}

NRF_STATIC_INLINE
nrf_regulators_pof_thrvddh_t nrf_regulators_pofcon_vddh_get(NRF_REGULATORS_Type const * p_reg)
{
    return (nrf_regulators_pof_thrvddh_t)((p_reg->POFCON & REGULATORS_POFCON_THRESHOLDVDDH_Msk) >>
                                          REGULATORS_POFCON_THRESHOLDVDDH_Pos);
}
#endif // NRF_REGULATORS_HAS_POFCON_VDDH

#if NRF_REGULATORS_HAS_DCDCEN_VDDH
NRF_STATIC_INLINE void nrf_regulators_dcdcen_vddh_set(NRF_REGULATORS_Type * p_reg, bool enable)
{
    p_reg->VREGH.DCDCEN = (enable) ? REGULATORS_VREGH_DCDCEN_DCDCEN_Enabled :
                                     REGULATORS_VREGH_DCDCEN_DCDCEN_Disabled;
}
#endif

#if NRF_REGULATORS_HAS_DCDCEN_RADIO
NRF_STATIC_INLINE void nrf_regulators_dcdcen_radio_set(NRF_REGULATORS_Type * p_reg, bool enable)
{
    p_reg->VREGRADIO.DCDCEN = (enable) ? REGULATORS_VREGRADIO_DCDCEN_DCDCEN_Enabled :
                                         REGULATORS_VREGRADIO_DCDCEN_DCDCEN_Disabled;
}
#endif

#endif // NRF_DECLARE_ONLY

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRF_REGULATORS_H__
