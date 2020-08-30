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

#ifndef NRF_FPU_H__
#define NRF_FPU_H__

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_fpu_hal FPU HAL
 * @{
 * @ingroup nrf_fpu
 * @brief   Hardware access layer (HAL) for managing the Floating Point Unit configuration.
 */

/** @brief FPU events. */
typedef enum
{
    NRF_FPU_EVENT_INVALIDOPERATION = offsetof(NRF_FPU_Type, EVENTS_INVALIDOPERATION), /**< An FPUIOC exception triggered by an invalid operation has occurred in the FPU. */
    NRF_FPU_EVENT_DIVIDEBYZERO     = offsetof(NRF_FPU_Type, EVENTS_DIVIDEBYZERO),     /**< An FPUDZC exception triggered by a floating-point divide-by-zero operation has occurred in the FPU. */
    NRF_FPU_EVENT_OVERFLOW         = offsetof(NRF_FPU_Type, EVENTS_OVERFLOW),         /**< An FPUOFC exception triggered by a floating-point overflow has occurred in the FPU. */
    NRF_FPU_EVENT_UNDERFLOW        = offsetof(NRF_FPU_Type, EVENTS_UNDERFLOW),        /**< An FPUUFC exception triggered by a floating-point underflow has occurred in the FPU. */
    NRF_FPU_EVENT_INEXACT          = offsetof(NRF_FPU_Type, EVENTS_INEXACT),          /**< An FPUIXC exception triggered by an inexact floating-point operation has occurred in the FPU. */
    NRF_FPU_EVENT_DENORMALINPUT    = offsetof(NRF_FPU_Type, EVENTS_DENORMALINPUT),    /**< An FPUIDC exception triggered by a denormal floating-point input has occurred in the FPU. */
} nrf_fpu_event_t;



/** @brief FPU interrupts. */
typedef enum
{
    NRF_FPU_INT_INVALIDOPERATION = FPU_INTEN_INVALIDOPERATION_Msk, /**< Interrupt on event INVALIDOPERATION. */
    NRF_FPU_INT_DIVIDEBYZERO     = FPU_INTEN_DIVIDEBYZERO_Msk,     /**< Interrupt on event DIVIDEBYZERO. */
    NRF_FPU_INT_OVERFLOW         = FPU_INTEN_OVERFLOW_Msk,         /**< Interrupt on event OVERFLOW. */
    NRF_FPU_INT_UNDERFLOW        = FPU_INTEN_UNDERFLOW_Msk,        /**< Interrupt on event UNDERFLOW. */
    NRF_FPU_INT_INEXACT          = FPU_INTEN_INEXACT_Msk,          /**< Interrupt on event INEXACT. */
    NRF_FPU_INT_DENORMALINPUT    = FPU_INTEN_DENORMALINPUT_Msk,    /**< Interrupt on event DENORMALINPUT. */
} nrf_fpu_int_mask_t;

/**
 * @brief Function for clearing the specified FPU event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event to be cleared.
 */
NRF_STATIC_INLINE void nrf_fpu_event_clear(NRF_FPU_Type *  p_reg,
                                           nrf_fpu_event_t event);

/**
 * @brief Function for retrieving the state of the FPU event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event to be checked.
 *
 * @retval true  The event has been generated.
 * @retval false The event has not been generated.
 */
NRF_STATIC_INLINE bool nrf_fpu_event_check(NRF_FPU_Type const * p_reg,
                                           nrf_fpu_event_t      event);

/**
 * @brief Function for enabling specified interrupts.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Mask of interrupts to be enabled.
 */
NRF_STATIC_INLINE void nrf_fpu_int_enable(NRF_FPU_Type * p_reg, uint32_t mask);

/**
 * @brief Function for disabling specified interrupts.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Mask of interrupts to be disabled.
 */
NRF_STATIC_INLINE void nrf_fpu_int_disable(NRF_FPU_Type * p_reg, uint32_t mask);

/**
 * @brief Function for checking if the specified interrupts are enabled.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Mask of interrupts to be checked.
 *
 * @return Mask of enabled interrupts.
 */
NRF_STATIC_INLINE uint32_t nrf_fpu_int_enable_check(NRF_FPU_Type const * p_reg, uint32_t mask);

#ifndef NRF_DECLARE_ONLY

NRF_STATIC_INLINE void nrf_fpu_event_clear(NRF_FPU_Type *  p_reg,
                                           nrf_fpu_event_t event)
{
    *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)event)) = 0x0UL;
}

NRF_STATIC_INLINE bool nrf_fpu_event_check(NRF_FPU_Type const * p_reg,
                                           nrf_fpu_event_t      event)
{
    return (bool)*(volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)event);
}

NRF_STATIC_INLINE void nrf_fpu_int_enable(NRF_FPU_Type * p_reg, uint32_t mask)
{
    p_reg->INTENSET = mask;
}

NRF_STATIC_INLINE void nrf_fpu_int_disable(NRF_FPU_Type * p_reg, uint32_t mask)
{
    p_reg->INTENCLR = mask;
}

NRF_STATIC_INLINE uint32_t nrf_fpu_int_enable_check(NRF_FPU_Type const * p_reg, uint32_t mask)
{
    return p_reg->INTENSET & mask;
}

#endif // NRF_DECLARE_ONLY

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRF_FPU_H__
