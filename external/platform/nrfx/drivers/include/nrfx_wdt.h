/*
 * Copyright (c) 2014 - 2020, Nordic Semiconductor ASA
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

#ifndef NRFX_WDT_H__
#define NRFX_WDT_H__

#include <nrfx.h>
#include <hal/nrf_wdt.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrfx_wdt WDT driver
 * @{
 * @ingroup nrf_wdt
 * @brief   Watchdog Timer (WDT) peripheral driver.
 */

#if !NRFX_CHECK(NRFX_WDT_CONFIG_NO_IRQ) || defined(__NRFX_DOXYGEN__)
/** @brief WDT instance interrupt priority configuration. */
    #define NRFX_WDT_IRQ_CONFIG .interrupt_priority = NRFX_WDT_DEFAULT_CONFIG_IRQ_PRIORITY
#else
    #define NRFX_WDT_IRQ_CONFIG
#endif

/** @brief WDT event handler function type. */
typedef void (*nrfx_wdt_event_handler_t)(void);

/** @brief WDT channel ID type. */
typedef nrf_wdt_rr_register_t nrfx_wdt_channel_id;

/** @brief Data structure of the Watchdog (WDT) driver instance. */
typedef struct
{
    NRF_WDT_Type * p_reg;        ///< Pointer to a structure with WDT registers.
    uint8_t        drv_inst_idx; ///< Index of the driver instance. For internal use only.
} nrfx_wdt_t;

#ifndef __NRFX_DOXYGEN__
enum {
#if NRFX_CHECK(NRFX_WDT0_ENABLED)
    NRFX_WDT0_INST_IDX,
#endif
#if NRFX_CHECK(NRFX_WDT1_ENABLED)
    NRFX_WDT1_INST_IDX,
#endif
    NRFX_WDT_ENABLED_COUNT
};
#endif

/** @brief Macro for creating an instance of the WDT driver. */
#define NRFX_WDT_INSTANCE(id)                               \
{                                                           \
    .p_reg        = NRF_WDT##id,                            \
    .drv_inst_idx = NRFX_CONCAT_3(NRFX_WDT, id, _INST_IDX), \
}

/**@brief Struct for WDT initialization. */
typedef struct
{
    nrf_wdt_behaviour_t    behaviour;          /**< WDT behaviour when CPU in sleep/halt mode. */
    uint32_t               reload_value;       /**< WDT reload value in ms. */
#if !NRFX_CHECK(NRFX_WDT_CONFIG_NO_IRQ) || defined(__NRFX_DOXYGEN__)
    uint8_t                interrupt_priority; /**< WDT interrupt priority */
#endif
} nrfx_wdt_config_t;

/**
 * @brief WDT driver default configuration.
 *
 * This configuration sets up WDT with the following options:
 * - run when CPU is in SLEEP mode, pause when in HALT mode
 * - reload value: 2000 ms
 */
#define NRFX_WDT_DEFAULT_CONFIG                          \
{                                                        \
    .behaviour          = NRF_WDT_BEHAVIOUR_RUN_SLEEP,   \
    .reload_value       = 2000,                          \
    NRFX_WDT_IRQ_CONFIG                                  \
}

/**
 * @brief Function for initializing the WDT driver instance.
 *
 * @param[in] p_instance        Pointer to the driver instance structure.
 * @param[in] p_config          Pointer to the structure with the initial configuration.
 * @param[in] wdt_event_handler Event handler provided by the user. Ignored when
 *                              @ref NRFX_WDT_CONFIG_NO_IRQ option is enabled.
 *
 * @retval NRFX_SUCCESS             Initialization was successful.
 * @retval NRFX_ERROR_INVALID_STATE The driver was already initialized.
 */
nrfx_err_t nrfx_wdt_init(nrfx_wdt_t const *        p_instance,
                         nrfx_wdt_config_t const * p_config,
                         nrfx_wdt_event_handler_t  wdt_event_handler);

/**
 * @brief Function for allocating a watchdog channel.
 *
 * @note This function can not be called after nrfx_wdt_start().
 *
 * @param[in]  p_instance   Pointer to the driver instance structure.
 * @param[out] p_channel_id ID of granted channel.
 *
 * @retval NRFX_SUCCESS      The channel was successfully allocated.
 * @retval NRFX_ERROR_NO_MEM There is no available channel to be used.
 */
nrfx_err_t nrfx_wdt_channel_alloc(nrfx_wdt_t const *    p_instance,
                                  nrfx_wdt_channel_id * p_channel_id);

/**
 * @brief Function for starting the watchdog.
 *
 * @note After calling this function the watchdog is started, so the user needs to feed
 *       all allocated watchdog channels to avoid reset. At least one watchdog channel
 *       must be allocated.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 */
void nrfx_wdt_enable(nrfx_wdt_t const * p_instance);

/**
 * @brief Function for feeding the watchdog.
 *
 * @details Function feeds all allocated watchdog channels.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 */
void nrfx_wdt_feed(nrfx_wdt_t const * p_instance);

/**
 * @brief Function for feeding an invidual watchdog channel.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 * @param[in] channel_id ID of watchdog channel.
 */
void nrfx_wdt_channel_feed(nrfx_wdt_t const * p_instance, nrfx_wdt_channel_id channel_id);

/**
 * @brief Function for returning a requested task address for the WDT driver module.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 * @param[in] task       One of the WDT tasks.
 *
 * @return Task address.
 */
NRFX_STATIC_INLINE uint32_t nrfx_wdt_task_address_get(nrfx_wdt_t const * p_instance,
                                                      nrf_wdt_task_t     task);

/**
 * @brief Function for returning a requested event address for the WDT driver module.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 * @param[in] event      One of the WDT events.
 *
 * @return Event address.
 */
NRFX_STATIC_INLINE uint32_t nrfx_wdt_event_address_get(nrfx_wdt_t const * p_instance,
                                                       nrf_wdt_event_t    event);


#ifndef NRFX_DECLARE_ONLY
NRFX_STATIC_INLINE uint32_t nrfx_wdt_task_address_get(nrfx_wdt_t const * p_instance,
                                                      nrf_wdt_task_t     task)
{
    return nrf_wdt_task_address_get(p_instance->p_reg, task);
}

NRFX_STATIC_INLINE uint32_t nrfx_wdt_event_address_get(nrfx_wdt_t const * p_instance,
                                                       nrf_wdt_event_t    event)
{
    return nrf_wdt_event_address_get(p_instance->p_reg, event);
}
#endif // NRFX_DECLARE_ONLY

/** @} */


void nrfx_wdt_0_irq_handler(void);
void nrfx_wdt_1_irq_handler(void);


#ifdef __cplusplus
}
#endif

#endif
