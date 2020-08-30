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

#ifndef NRF_VREQCTRL_H__
#define NRF_VREQCTRL_H__

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_vreqctrl_hal VREQCTRL HAL
 * @{
 * @ingroup nrf_power
 * @brief   Hardware access layer for managing the VREQCTRL peripheral.
 */

/**
 * @brief Function for requesting high voltage on RADIO, to increase its output power by 3 dBm.
 *
 * @param[in] p_reg  Pointer to the structure of registers of the peripheral.
 * @param[in] enable True if high voltage on RADIO is to be enabled, false otherwise.
 */
NRF_STATIC_INLINE void nrf_vreqctrl_radio_high_voltage_set(NRF_VREQCTRL_Type * p_reg, bool enable);

/**
 * @brief Function for checking if high voltage on RADIO is ready.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @retval true  The high voltage is ready.
 * @retval false The high voltage is not ready.
 */
NRF_STATIC_INLINE bool nrf_vreqctrl_radio_high_voltage_check(NRF_VREQCTRL_Type const * p_reg);

#ifndef NRF_DECLARE_ONLY

NRF_STATIC_INLINE void nrf_vreqctrl_radio_high_voltage_set(NRF_VREQCTRL_Type * p_reg, bool enable)
{
    p_reg->VREGRADIO.VREQH =
        (enable ? VREQCTRL_VREGRADIO_VREQH_VREQH_Enabled : VREQCTRL_VREGRADIO_VREQH_VREQH_Disabled);
}

NRF_STATIC_INLINE bool nrf_vreqctrl_radio_high_voltage_check(NRF_VREQCTRL_Type const * p_reg)
{
    return (bool)(p_reg->VREGRADIO.VREQHREADY & VREQCTRL_VREGRADIO_VREQHREADY_READY_Msk);
}

#endif // NRF_DECLARE_ONLY

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRF_VREQCTRL_H__
