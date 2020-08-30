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

#ifndef NRF_MUTEX_H__
#define NRF_MUTEX_H__

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_mutex_hal MUTEX HAL
 * @{
 * @ingroup nrf_mutex
 * @brief   Hardware access layer for managing the MUTEX peripheral.
 */

/**
 * @brief Function for locking the specified mutex.
 *
 * If the specified mutex is already locked, its state remains unchanged.
 *
 * @note Faults are not managed by the MUTEX peripheral.
 *       One consequence is that if a mutex is locked and a fault happens,
 *       it is the responsibility of the fault handler to release the mutex.
 *       If a fault handler is not managing the mutex release, the mutex will remain locked.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mutex Index of the mutex to be locked.
 *
 * @retval true  Mutex is successfully locked.
 * @retval false Mutex was already locked.
 */
NRF_STATIC_INLINE bool nrf_mutex_lock(NRF_MUTEX_Type * p_reg, uint8_t mutex);

/**
 * @brief Function for unlocking the specified mutex.
 *
 * If the specified mutex is already unlocked, its state remains unchanged.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mutex Index of the mutex to be locked.
 */
NRF_STATIC_INLINE void nrf_mutex_unlock(NRF_MUTEX_Type * p_reg, uint8_t mutex);

#ifndef NRF_DECLARE_ONLY

NRF_STATIC_INLINE bool nrf_mutex_lock(NRF_MUTEX_Type * p_reg, uint8_t mutex)
{
    return (p_reg->MUTEX[mutex] == MUTEX_MUTEX_MUTEX_Unlocked);
}

NRF_STATIC_INLINE void nrf_mutex_unlock(NRF_MUTEX_Type * p_reg, uint8_t mutex)
{
    p_reg->MUTEX[mutex] = MUTEX_MUTEX_MUTEX_Unlocked;
}

#endif // NRF_DECLARE_ONLY

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRF_MUTEX_H__
