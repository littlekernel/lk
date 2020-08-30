/*
 * Copyright (c) 2017 - 2020, Nordic Semiconductor ASA
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

#ifndef NRF_POWER_H__
#define NRF_POWER_H__

#include <nrfx.h>
#include <nrf_erratas.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_power_hal POWER HAL
 * @{
 * @ingroup nrf_power
 * @brief   Hardware access layer for managing the POWER peripheral.
 */

#if defined(POWER_INTENSET_SLEEPENTER_Msk) || defined(__NRFX_DOXYGEN__)
/** @brief Symbol indicating whether sleep events are present. */
#define NRF_POWER_HAS_SLEEPEVT 1
#else
#define NRF_POWER_HAS_SLEEPEVT 0
#endif

#if defined(POWER_USBREGSTATUS_VBUSDETECT_Msk) || defined(__NRFX_DOXYGEN__)
/** @brief Symbol indicating whether the POWER peripheral controls the USB regulator. */
#define NRF_POWER_HAS_USBREG 1
#else
#define NRF_POWER_HAS_USBREG 0
#endif

#if defined(POWER_DCDCEN0_DCDCEN_Msk) || defined(__NRFX_DOXYGEN__)
/** @brief Symbol indicating whether DCDCEN for REG0 is present. */
#define NRF_POWER_HAS_DCDCEN_VDDH 1
#else
#define NRF_POWER_HAS_DCDCEN_VDDH 0
#endif

#if defined(POWER_DCDCEN_DCDCEN_Msk) || defined(__NRFX_DOXYGEN__)
/** @brief Symbol indicating whether DCDCEN for REG1 is present. */
#define NRF_POWER_HAS_DCDCEN 1
#else
#define NRF_POWER_HAS_DCDCEN 0
#endif

#if defined(POWER_INTENSET_POFWARN_Msk) || defined(__NRFX_DOXYGEN__)
/** @brief Symbol indicating whether power failure event is present. */
#define NRF_POWER_HAS_POFWARN 1
#else
#define NRF_POWER_HAS_POFWARN 0
#endif

#if defined(POWER_POFCON_THRESHOLD_Msk) || defined(__NRFX_DOXYGEN__)
/** @brief Symbol indicating whether power failure comparator is present. */
#define NRF_POWER_HAS_POFCON 1
#else
#define NRF_POWER_HAS_POFCON 0
#endif

#if defined(POWER_POFCON_THRESHOLDVDDH_Msk) || defined(__NRFX_DOXYGEN__)
/** @brief Symbol indicating whether power failure comparator for VDDH is present. */
#define NRF_POWER_HAS_POFCON_VDDH 1
#else
#define NRF_POWER_HAS_POFCON_VDDH 0
#endif

#if defined(POWER_RESETREAS_RESETPIN_Msk) || defined(__NRFX_DOXYGEN__)
/** @brief Auxiliary definition to mark the fact that RESETREAS register is present in POWER */
#define NRF_POWER_HAS_RESETREAS 1
#else
#define NRF_POWER_HAS_RESETREAS 0
#endif

#if defined(POWER_MAINREGSTATUS_MAINREGSTATUS_Msk) || defined(__NRFX_DOXYGEN__)
/** @brief Symbol indicating whether MAINREGSTATUS register is present. */
#define NRF_POWER_HAS_MAINREGSTATUS 1
#else
#define NRF_POWER_HAS_MAINREGSTATUS 0
#endif

/** @brief POWER tasks. */
typedef enum
{
    NRF_POWER_TASK_CONSTLAT  = offsetof(NRF_POWER_Type, TASKS_CONSTLAT), /**< Enable constant latency mode. */
    NRF_POWER_TASK_LOWPWR    = offsetof(NRF_POWER_Type, TASKS_LOWPWR  ), /**< Enable low-power mode (variable latency). */
} nrf_power_task_t;

/** @brief POWER events. */
typedef enum
{
#if NRF_POWER_HAS_POFWARN
    NRF_POWER_EVENT_POFWARN      = offsetof(NRF_POWER_Type, EVENTS_POFWARN    ), /**< Power failure warning. */
#endif
#if NRF_POWER_HAS_SLEEPEVT
    NRF_POWER_EVENT_SLEEPENTER   = offsetof(NRF_POWER_Type, EVENTS_SLEEPENTER ), /**< CPU entered WFI/WFE sleep. */
    NRF_POWER_EVENT_SLEEPEXIT    = offsetof(NRF_POWER_Type, EVENTS_SLEEPEXIT  ), /**< CPU exited WFI/WFE sleep. */
#endif
#if NRF_POWER_HAS_USBREG
    NRF_POWER_EVENT_USBDETECTED  = offsetof(NRF_POWER_Type, EVENTS_USBDETECTED), /**< Voltage supply detected on VBUS. */
    NRF_POWER_EVENT_USBREMOVED   = offsetof(NRF_POWER_Type, EVENTS_USBREMOVED ), /**< Voltage supply removed from VBUS. */
    NRF_POWER_EVENT_USBPWRRDY    = offsetof(NRF_POWER_Type, EVENTS_USBPWRRDY  ), /**< USB 3.3&nbsp;V supply ready. */
#endif
} nrf_power_event_t;

/** @brief POWER interrupts. */
typedef enum
{
#if NRF_POWER_HAS_POFWARN
    NRF_POWER_INT_POFWARN_MASK     = POWER_INTENSET_POFWARN_Msk    , /**< Write '1' to Enable interrupt for POFWARN event. */
#endif
#if NRF_POWER_HAS_SLEEPEVT
    NRF_POWER_INT_SLEEPENTER_MASK  = POWER_INTENSET_SLEEPENTER_Msk , /**< Write '1' to Enable interrupt for SLEEPENTER event. */
    NRF_POWER_INT_SLEEPEXIT_MASK   = POWER_INTENSET_SLEEPEXIT_Msk  , /**< Write '1' to Enable interrupt for SLEEPEXIT event. */
#endif
#if NRF_POWER_HAS_USBREG
    NRF_POWER_INT_USBDETECTED_MASK = POWER_INTENSET_USBDETECTED_Msk, /**< Write '1' to Enable interrupt for USBDETECTED event. */
    NRF_POWER_INT_USBREMOVED_MASK  = POWER_INTENSET_USBREMOVED_Msk , /**< Write '1' to Enable interrupt for USBREMOVED event. */
    NRF_POWER_INT_USBPWRRDY_MASK   = POWER_INTENSET_USBPWRRDY_Msk  , /**< Write '1' to Enable interrupt for USBPWRRDY event. */
#endif
} nrf_power_int_mask_t;

/** @brief Reset reason. */
#if NRF_POWER_HAS_RESETREAS
typedef enum
{
    NRF_POWER_RESETREAS_RESETPIN_MASK = POWER_RESETREAS_RESETPIN_Msk, /**< Bit mask of RESETPIN field. */
    NRF_POWER_RESETREAS_DOG_MASK      = POWER_RESETREAS_DOG_Msk     , /**< Bit mask of DOG field. */
    NRF_POWER_RESETREAS_SREQ_MASK     = POWER_RESETREAS_SREQ_Msk    , /**< Bit mask of SREQ field. */
    NRF_POWER_RESETREAS_LOCKUP_MASK   = POWER_RESETREAS_LOCKUP_Msk  , /**< Bit mask of LOCKUP field. */
    NRF_POWER_RESETREAS_OFF_MASK      = POWER_RESETREAS_OFF_Msk     , /**< Bit mask of OFF field. */
#if defined(POWER_RESETREAS_LPCOMP_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_POWER_RESETREAS_LPCOMP_MASK   = POWER_RESETREAS_LPCOMP_Msk  , /**< Bit mask of LPCOMP field. */
#endif
    NRF_POWER_RESETREAS_DIF_MASK      = POWER_RESETREAS_DIF_Msk     , /**< Bit mask of DIF field. */
#if defined(POWER_RESETREAS_NFC_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_POWER_RESETREAS_NFC_MASK      = POWER_RESETREAS_NFC_Msk     , /**< Bit mask of NFC field. */
#endif
#if defined(POWER_RESETREAS_VBUS_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_POWER_RESETREAS_VBUS_MASK     = POWER_RESETREAS_VBUS_Msk    , /**< Bit mask of VBUS field. */
#endif
} nrf_power_resetreas_mask_t;
#endif // NRF_POWER_HAS_RESETREAS

#if NRF_POWER_HAS_USBREG
/**
 * @brief USBREGSTATUS register bit masks
 *
 * @sa nrf_power_usbregstatus_get
 */
typedef enum
{
    NRF_POWER_USBREGSTATUS_VBUSDETECT_MASK = POWER_USBREGSTATUS_VBUSDETECT_Msk, /**< USB detected or removed.     */
    NRF_POWER_USBREGSTATUS_OUTPUTRDY_MASK  = POWER_USBREGSTATUS_OUTPUTRDY_Msk   /**< USB 3.3&nbsp;V supply ready. */
} nrf_power_usbregstatus_mask_t;
#endif // NRF_POWER_HAS_USBREG

#if defined(POWER_RAMSTATUS_RAMBLOCK0_Msk) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Numbers of RAM blocks
 *
 * @sa nrf_power_ramblock_mask_t
 * @note
 * RAM blocks must be used in nRF51.
 * In newer SoCs, RAM is divided into segments and this functionality is not supported.
 * See the PS for mapping between the internal RAM and RAM blocks, because this
 * mapping is not 1:1, and functions related to old style blocks must not be used.
 */
typedef enum
{
    NRF_POWER_RAMBLOCK0 = POWER_RAMSTATUS_RAMBLOCK0_Pos,
    NRF_POWER_RAMBLOCK1 = POWER_RAMSTATUS_RAMBLOCK1_Pos,
#if defined(POWER_RAMSTATUS_RAMBLOCK2_Pos) ||  defined(__NRFX_DOXYGEN__)
    NRF_POWER_RAMBLOCK2 = POWER_RAMSTATUS_RAMBLOCK2_Pos,
#endif
#if defined(POWER_RAMSTATUS_RAMBLOCK3_Pos) ||  defined(__NRFX_DOXYGEN__)
    NRF_POWER_RAMBLOCK3 = POWER_RAMSTATUS_RAMBLOCK3_Pos
#endif
} nrf_power_ramblock_t;

/**
 * @brief Masks of RAM blocks.
 *
 * @sa nrf_power_ramblock_t
 */
typedef enum
{
    NRF_POWER_RAMBLOCK0_MASK = POWER_RAMSTATUS_RAMBLOCK0_Msk,
    NRF_POWER_RAMBLOCK1_MASK = POWER_RAMSTATUS_RAMBLOCK1_Msk,
#if defined(POWER_RAMSTATUS_RAMBLOCK2_Msk) ||  defined(__NRFX_DOXYGEN__)
    NRF_POWER_RAMBLOCK2_MASK = POWER_RAMSTATUS_RAMBLOCK2_Msk,
#endif
#if defined(POWER_RAMSTATUS_RAMBLOCK3_Msk) ||  defined(__NRFX_DOXYGEN__)
    NRF_POWER_RAMBLOCK3_MASK = POWER_RAMSTATUS_RAMBLOCK3_Msk
#endif
} nrf_power_ramblock_mask_t;
#endif // defined(POWER_RAMSTATUS_RAMBLOCK0_Msk) || defined(__NRFX_DOXYGEN__)

/**
 * @brief RAM power state position of the bits
 *
 * @sa nrf_power_onoffram_mask_t
 */
typedef enum
{
    NRF_POWER_ONRAM0,  /**< Keep RAM block 0 ON or OFF in System ON mode.                 */
    NRF_POWER_OFFRAM0, /**< Keep retention on RAM block 0 when RAM block is switched OFF. */
    NRF_POWER_ONRAM1,  /**< Keep RAM block 1 ON or OFF in System ON mode.                 */
    NRF_POWER_OFFRAM1, /**< Keep retention on RAM block 1 when RAM block is switched OFF. */
    NRF_POWER_ONRAM2,  /**< Keep RAM block 2 ON or OFF in System ON mode.                 */
    NRF_POWER_OFFRAM2, /**< Keep retention on RAM block 2 when RAM block is switched OFF. */
    NRF_POWER_ONRAM3,  /**< Keep RAM block 3 ON or OFF in System ON mode.                 */
    NRF_POWER_OFFRAM3, /**< Keep retention on RAM block 3 when RAM block is switched OFF. */
}nrf_power_onoffram_t;

/**
 * @brief RAM power state bit masks
 *
 * @sa nrf_power_onoffram_t
 */
typedef enum
{
    NRF_POWER_ONRAM0_MASK  = 1U << NRF_POWER_ONRAM0,  /**< Keep RAM block 0 ON or OFF in System ON mode.                 */
    NRF_POWER_OFFRAM0_MASK = 1U << NRF_POWER_OFFRAM0, /**< Keep retention on RAM block 0 when RAM block is switched OFF. */
    NRF_POWER_ONRAM1_MASK  = 1U << NRF_POWER_ONRAM1,  /**< Keep RAM block 1 ON or OFF in System ON mode.                 */
    NRF_POWER_OFFRAM1_MASK = 1U << NRF_POWER_OFFRAM1, /**< Keep retention on RAM block 1 when RAM block is switched OFF. */
    NRF_POWER_ONRAM2_MASK  = 1U << NRF_POWER_ONRAM2,  /**< Keep RAM block 2 ON or OFF in System ON mode.                 */
    NRF_POWER_OFFRAM2_MASK = 1U << NRF_POWER_OFFRAM2, /**< Keep retention on RAM block 2 when RAM block is switched OFF. */
    NRF_POWER_ONRAM3_MASK  = 1U << NRF_POWER_ONRAM3,  /**< Keep RAM block 3 ON or OFF in System ON mode.                 */
    NRF_POWER_OFFRAM3_MASK = 1U << NRF_POWER_OFFRAM3, /**< Keep retention on RAM block 3 when RAM block is switched OFF. */
}nrf_power_onoffram_mask_t;

#if NRF_POWER_HAS_POFCON
/** @brief Power failure comparator thresholds. */
typedef enum
{
    NRF_POWER_POFTHR_V21 = POWER_POFCON_THRESHOLD_V21, /**< Set threshold to 2.1&nbsp;V. */
    NRF_POWER_POFTHR_V23 = POWER_POFCON_THRESHOLD_V23, /**< Set threshold to 2.3&nbsp;V. */
    NRF_POWER_POFTHR_V25 = POWER_POFCON_THRESHOLD_V25, /**< Set threshold to 2.5&nbsp;V. */
    NRF_POWER_POFTHR_V27 = POWER_POFCON_THRESHOLD_V27, /**< Set threshold to 2.7&nbsp;V. */
#if defined(POWER_POFCON_THRESHOLD_V17) || defined(__NRFX_DOXYGEN__)
    NRF_POWER_POFTHR_V17 = POWER_POFCON_THRESHOLD_V17, /**< Set threshold to 1.7&nbsp;V. */
    NRF_POWER_POFTHR_V18 = POWER_POFCON_THRESHOLD_V18, /**< Set threshold to 1.8&nbsp;V. */
    NRF_POWER_POFTHR_V19 = POWER_POFCON_THRESHOLD_V19, /**< Set threshold to 1.9&nbsp;V. */
    NRF_POWER_POFTHR_V20 = POWER_POFCON_THRESHOLD_V20, /**< Set threshold to 2.0&nbsp;V. */
    NRF_POWER_POFTHR_V22 = POWER_POFCON_THRESHOLD_V22, /**< Set threshold to 2.2&nbsp;V. */
    NRF_POWER_POFTHR_V24 = POWER_POFCON_THRESHOLD_V24, /**< Set threshold to 2.4&nbsp;V. */
    NRF_POWER_POFTHR_V26 = POWER_POFCON_THRESHOLD_V26, /**< Set threshold to 2.6&nbsp;V. */
    NRF_POWER_POFTHR_V28 = POWER_POFCON_THRESHOLD_V28, /**< Set threshold to 2.8&nbsp;V. */
#endif // defined(POWER_POFCON_THRESHOLD_V17) || defined(__NRFX_DOXYGEN__)
} nrf_power_pof_thr_t;
#endif // NRF_POWER_HAS_POFCON

#if NRF_POWER_HAS_POFCON_VDDH
/** @brief Power failure comparator thresholds for VDDH. */
typedef enum
{
    NRF_POWER_POFTHRVDDH_V27 = POWER_POFCON_THRESHOLDVDDH_V27, /**< Set threshold to 2.7&nbsp;V. */
    NRF_POWER_POFTHRVDDH_V28 = POWER_POFCON_THRESHOLDVDDH_V28, /**< Set threshold to 2.8&nbsp;V. */
    NRF_POWER_POFTHRVDDH_V29 = POWER_POFCON_THRESHOLDVDDH_V29, /**< Set threshold to 2.9&nbsp;V. */
    NRF_POWER_POFTHRVDDH_V30 = POWER_POFCON_THRESHOLDVDDH_V30, /**< Set threshold to 3.0&nbsp;V. */
    NRF_POWER_POFTHRVDDH_V31 = POWER_POFCON_THRESHOLDVDDH_V31, /**< Set threshold to 3.1&nbsp;V. */
    NRF_POWER_POFTHRVDDH_V32 = POWER_POFCON_THRESHOLDVDDH_V32, /**< Set threshold to 3.2&nbsp;V. */
    NRF_POWER_POFTHRVDDH_V33 = POWER_POFCON_THRESHOLDVDDH_V33, /**< Set threshold to 3.3&nbsp;V. */
    NRF_POWER_POFTHRVDDH_V34 = POWER_POFCON_THRESHOLDVDDH_V34, /**< Set threshold to 3.4&nbsp;V. */
    NRF_POWER_POFTHRVDDH_V35 = POWER_POFCON_THRESHOLDVDDH_V35, /**< Set threshold to 3.5&nbsp;V. */
    NRF_POWER_POFTHRVDDH_V36 = POWER_POFCON_THRESHOLDVDDH_V36, /**< Set threshold to 3.6&nbsp;V. */
    NRF_POWER_POFTHRVDDH_V37 = POWER_POFCON_THRESHOLDVDDH_V37, /**< Set threshold to 3.7&nbsp;V. */
    NRF_POWER_POFTHRVDDH_V38 = POWER_POFCON_THRESHOLDVDDH_V38, /**< Set threshold to 3.8&nbsp;V. */
    NRF_POWER_POFTHRVDDH_V39 = POWER_POFCON_THRESHOLDVDDH_V39, /**< Set threshold to 3.9&nbsp;V. */
    NRF_POWER_POFTHRVDDH_V40 = POWER_POFCON_THRESHOLDVDDH_V40, /**< Set threshold to 4.0&nbsp;V. */
    NRF_POWER_POFTHRVDDH_V41 = POWER_POFCON_THRESHOLDVDDH_V41, /**< Set threshold to 4.1&nbsp;V. */
    NRF_POWER_POFTHRVDDH_V42 = POWER_POFCON_THRESHOLDVDDH_V42, /**< Set threshold to 4.2&nbsp;V. */
} nrf_power_pof_thrvddh_t;
#endif // NRF_POWER_HAS_POFCON_VDDH

#if NRF_POWER_HAS_MAINREGSTATUS
/** @brief Main regulator status. */
typedef enum
{
    NRF_POWER_MAINREGSTATUS_NORMAL = POWER_MAINREGSTATUS_MAINREGSTATUS_Normal, /**< Normal voltage mode. Voltage supplied on VDD. */
    NRF_POWER_MAINREGSTATUS_HIGH   = POWER_MAINREGSTATUS_MAINREGSTATUS_High    /**< High voltage mode. Voltage supplied on VDDH.  */
} nrf_power_mainregstatus_t;
#endif

#if defined(POWER_RAM_POWER_S0POWER_Msk) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Bit positions for RAMPOWER register
 *
 * All possible bits described, even if they are not used in selected MCU.
 */
typedef enum
{
    /** Keep RAM section S0 ON in System ON mode */
    NRF_POWER_RAMPOWER_S0POWER = POWER_RAM_POWER_S0POWER_Pos,
    NRF_POWER_RAMPOWER_S1POWER,  /**< Keep RAM section S1 ON in System ON mode. */
    NRF_POWER_RAMPOWER_S2POWER,  /**< Keep RAM section S2 ON in System ON mode. */
    NRF_POWER_RAMPOWER_S3POWER,  /**< Keep RAM section S3 ON in System ON mode. */
    NRF_POWER_RAMPOWER_S4POWER,  /**< Keep RAM section S4 ON in System ON mode. */
    NRF_POWER_RAMPOWER_S5POWER,  /**< Keep RAM section S5 ON in System ON mode. */
    NRF_POWER_RAMPOWER_S6POWER,  /**< Keep RAM section S6 ON in System ON mode. */
    NRF_POWER_RAMPOWER_S7POWER,  /**< Keep RAM section S7 ON in System ON mode. */
    NRF_POWER_RAMPOWER_S8POWER,  /**< Keep RAM section S8 ON in System ON mode. */
    NRF_POWER_RAMPOWER_S9POWER,  /**< Keep RAM section S9 ON in System ON mode. */
    NRF_POWER_RAMPOWER_S10POWER, /**< Keep RAM section S10 ON in System ON mode. */
    NRF_POWER_RAMPOWER_S11POWER, /**< Keep RAM section S11 ON in System ON mode. */
    NRF_POWER_RAMPOWER_S12POWER, /**< Keep RAM section S12 ON in System ON mode. */
    NRF_POWER_RAMPOWER_S13POWER, /**< Keep RAM section S13 ON in System ON mode. */
    NRF_POWER_RAMPOWER_S14POWER, /**< Keep RAM section S14 ON in System ON mode. */
    NRF_POWER_RAMPOWER_S15POWER, /**< Keep RAM section S15 ON in System ON mode. */

    /** Keep section retention in OFF mode when section is OFF */
    NRF_POWER_RAMPOWER_S0RETENTION = POWER_RAM_POWER_S0RETENTION_Pos,
    NRF_POWER_RAMPOWER_S1RETENTION,  /**< Keep section retention in OFF mode when section is OFF. */
    NRF_POWER_RAMPOWER_S2RETENTION,  /**< Keep section retention in OFF mode when section is OFF. */
    NRF_POWER_RAMPOWER_S3RETENTION,  /**< Keep section retention in OFF mode when section is OFF. */
    NRF_POWER_RAMPOWER_S4RETENTION,  /**< Keep section retention in OFF mode when section is OFF. */
    NRF_POWER_RAMPOWER_S5RETENTION,  /**< Keep section retention in OFF mode when section is OFF. */
    NRF_POWER_RAMPOWER_S6RETENTION,  /**< Keep section retention in OFF mode when section is OFF. */
    NRF_POWER_RAMPOWER_S7RETENTION,  /**< Keep section retention in OFF mode when section is OFF. */
    NRF_POWER_RAMPOWER_S8RETENTION,  /**< Keep section retention in OFF mode when section is OFF. */
    NRF_POWER_RAMPOWER_S9RETENTION,  /**< Keep section retention in OFF mode when section is OFF. */
    NRF_POWER_RAMPOWER_S10RETENTION, /**< Keep section retention in OFF mode when section is OFF. */
    NRF_POWER_RAMPOWER_S11RETENTION, /**< Keep section retention in OFF mode when section is OFF. */
    NRF_POWER_RAMPOWER_S12RETENTION, /**< Keep section retention in OFF mode when section is OFF. */
    NRF_POWER_RAMPOWER_S13RETENTION, /**< Keep section retention in OFF mode when section is OFF. */
    NRF_POWER_RAMPOWER_S14RETENTION, /**< Keep section retention in OFF mode when section is OFF. */
    NRF_POWER_RAMPOWER_S15RETENTION, /**< Keep section retention in OFF mode when section is OFF. */
} nrf_power_rampower_t;

/**
 * @brief Bit masks for RAMPOWER register
 *
 * All possible bits described, even if they are not used in selected MCU.
 */
typedef enum
{
    NRF_POWER_RAMPOWER_S0POWER_MASK  = 1UL << NRF_POWER_RAMPOWER_S0POWER ,
    NRF_POWER_RAMPOWER_S1POWER_MASK  = 1UL << NRF_POWER_RAMPOWER_S1POWER ,
    NRF_POWER_RAMPOWER_S2POWER_MASK  = 1UL << NRF_POWER_RAMPOWER_S2POWER ,
    NRF_POWER_RAMPOWER_S3POWER_MASK  = 1UL << NRF_POWER_RAMPOWER_S3POWER ,
    NRF_POWER_RAMPOWER_S4POWER_MASK  = 1UL << NRF_POWER_RAMPOWER_S4POWER ,
    NRF_POWER_RAMPOWER_S5POWER_MASK  = 1UL << NRF_POWER_RAMPOWER_S5POWER ,
    NRF_POWER_RAMPOWER_S7POWER_MASK  = 1UL << NRF_POWER_RAMPOWER_S7POWER ,
    NRF_POWER_RAMPOWER_S8POWER_MASK  = 1UL << NRF_POWER_RAMPOWER_S8POWER ,
    NRF_POWER_RAMPOWER_S9POWER_MASK  = 1UL << NRF_POWER_RAMPOWER_S9POWER ,
    NRF_POWER_RAMPOWER_S10POWER_MASK = 1UL << NRF_POWER_RAMPOWER_S10POWER,
    NRF_POWER_RAMPOWER_S11POWER_MASK = 1UL << NRF_POWER_RAMPOWER_S11POWER,
    NRF_POWER_RAMPOWER_S12POWER_MASK = 1UL << NRF_POWER_RAMPOWER_S12POWER,
    NRF_POWER_RAMPOWER_S13POWER_MASK = 1UL << NRF_POWER_RAMPOWER_S13POWER,
    NRF_POWER_RAMPOWER_S14POWER_MASK = 1UL << NRF_POWER_RAMPOWER_S14POWER,
    NRF_POWER_RAMPOWER_S15POWER_MASK = 1UL << NRF_POWER_RAMPOWER_S15POWER,

    NRF_POWER_RAMPOWER_S0RETENTION_MASK  = 1UL << NRF_POWER_RAMPOWER_S0RETENTION ,
    NRF_POWER_RAMPOWER_S1RETENTION_MASK  = 1UL << NRF_POWER_RAMPOWER_S1RETENTION ,
    NRF_POWER_RAMPOWER_S2RETENTION_MASK  = 1UL << NRF_POWER_RAMPOWER_S2RETENTION ,
    NRF_POWER_RAMPOWER_S3RETENTION_MASK  = 1UL << NRF_POWER_RAMPOWER_S3RETENTION ,
    NRF_POWER_RAMPOWER_S4RETENTION_MASK  = 1UL << NRF_POWER_RAMPOWER_S4RETENTION ,
    NRF_POWER_RAMPOWER_S5RETENTION_MASK  = 1UL << NRF_POWER_RAMPOWER_S5RETENTION ,
    NRF_POWER_RAMPOWER_S7RETENTION_MASK  = 1UL << NRF_POWER_RAMPOWER_S7RETENTION ,
    NRF_POWER_RAMPOWER_S8RETENTION_MASK  = 1UL << NRF_POWER_RAMPOWER_S8RETENTION ,
    NRF_POWER_RAMPOWER_S9RETENTION_MASK  = 1UL << NRF_POWER_RAMPOWER_S9RETENTION ,
    NRF_POWER_RAMPOWER_S10RETENTION_MASK = 1UL << NRF_POWER_RAMPOWER_S10RETENTION,
    NRF_POWER_RAMPOWER_S11RETENTION_MASK = 1UL << NRF_POWER_RAMPOWER_S11RETENTION,
    NRF_POWER_RAMPOWER_S12RETENTION_MASK = 1UL << NRF_POWER_RAMPOWER_S12RETENTION,
    NRF_POWER_RAMPOWER_S13RETENTION_MASK = 1UL << NRF_POWER_RAMPOWER_S13RETENTION,
    NRF_POWER_RAMPOWER_S14RETENTION_MASK = 1UL << NRF_POWER_RAMPOWER_S14RETENTION,
    NRF_POWER_RAMPOWER_S15RETENTION_MASK = (int)(1UL << NRF_POWER_RAMPOWER_S15RETENTION),
} nrf_power_rampower_mask_t;
#endif // defined(POWER_RAM_POWER_S0POWER_Msk) || defined(__NRFX_DOXYGEN__)

/**
 * @brief Function for activating a specific POWER task.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  Task.
 */
NRF_STATIC_INLINE void nrf_power_task_trigger(NRF_POWER_Type * p_reg, nrf_power_task_t task);

/**
 * @brief Function for returning the address of a specific POWER task register.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  Task.
 *
 * @return Task address.
 */
NRF_STATIC_INLINE uint32_t nrf_power_task_address_get(NRF_POWER_Type const * p_reg,
                                                      nrf_power_task_t       task);

/**
 * @brief Function for clearing a specific event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event.
 */
NRF_STATIC_INLINE void nrf_power_event_clear(NRF_POWER_Type * p_reg, nrf_power_event_t event);

/**
 * @brief Function for retrieving the state of the POWER event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event to be checked.
 *
 * @retval true  The event has been generated.
 * @retval false The event has not been generated.
 */
NRF_STATIC_INLINE bool nrf_power_event_check(NRF_POWER_Type const * p_reg, nrf_power_event_t event);

/**
 * @brief Function for getting and clearing the state of specific event
 *
 * This function checks the state of the event and clears it.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event.
 *
 * @retval true  The event was set.
 * @retval false The event was not set.
 */
NRF_STATIC_INLINE bool nrf_power_event_get_and_clear(NRF_POWER_Type *  p_reg,
                                                     nrf_power_event_t event);

/**
 * @brief Function for returning the address of a specific POWER event register.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event.
 *
 * @return Address.
 */
NRF_STATIC_INLINE uint32_t nrf_power_event_address_get(NRF_POWER_Type const * p_reg,
                                                       nrf_power_event_t      event);

/**
 * @brief Function for enabling selected interrupts.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Mask of interrupts to be enabled.
 */
NRF_STATIC_INLINE void nrf_power_int_enable(NRF_POWER_Type * p_reg, uint32_t mask);

/**
 * @brief Function for checking if the specified interrupts are enabled.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Mask of interrupts to be checked.
 *
 * @return Mask of enabled interrupts.
 */
NRF_STATIC_INLINE uint32_t nrf_power_int_enable_check(NRF_POWER_Type const * p_reg, uint32_t mask);

/**
 * @brief Function for retrieving the information about enabled interrupts.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return The flags of enabled interrupts.
 */
NRF_STATIC_INLINE uint32_t nrf_power_int_enable_get(NRF_POWER_Type const * p_reg);

/**
 * @brief Function for disabling selected interrupts.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Mask of interrupts to be disabled.
 */
NRF_STATIC_INLINE void nrf_power_int_disable(NRF_POWER_Type * p_reg, uint32_t mask);

#if defined(DPPI_PRESENT) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for setting the subscribe configuration for a given
 *        POWER task.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] task    Task for which to set the configuration.
 * @param[in] channel Channel through which to subscribe events.
 */
NRF_STATIC_INLINE void nrf_power_subscribe_set(NRF_POWER_Type * p_reg,
                                               nrf_power_task_t task,
                                               uint8_t          channel);

/**
 * @brief Function for clearing the subscribe configuration for a given
 *        POWER task.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  Task for which to clear the configuration.
 */
NRF_STATIC_INLINE void nrf_power_subscribe_clear(NRF_POWER_Type * p_reg, nrf_power_task_t task);

/**
 * @brief Function for setting the publish configuration for a given
 *        POWER event.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] event   Event for which to set the configuration.
 * @param[in] channel Channel through which to publish the event.
 */
NRF_STATIC_INLINE void nrf_power_publish_set(NRF_POWER_Type *  p_reg,
                                             nrf_power_event_t event,
                                             uint8_t           channel);

/**
 * @brief Function for clearing the publish configuration for a given
 *        POWER event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event for which to clear the configuration.
 */
NRF_STATIC_INLINE void nrf_power_publish_clear(NRF_POWER_Type * p_reg, nrf_power_event_t event);
#endif // defined(DPPI_PRESENT) || defined(__NRFX_DOXYGEN__)

#if NRF_POWER_HAS_RESETREAS
/**
 * @brief Function for getting the reset reason bitmask.
 *
 * This function returns the reset reason bitmask.
 * Unless cleared, the RESETREAS register is cumulative.
 * A field is cleared by writing '1' to it (see @ref nrf_power_resetreas_clear).
 * If none of the reset sources is flagged,
 * the chip was reset from the on-chip reset generator,
 * which indicates a power-on-reset or a brown out reset.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return The mask of reset reasons constructed with @ref nrf_power_resetreas_mask_t.
 */
NRF_STATIC_INLINE uint32_t nrf_power_resetreas_get(NRF_POWER_Type const * p_reg);

/**
 * @brief Function for clearing the selected reset reason field.
 *
 * This function clears the selected reset reason field.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  The mask constructed from @ref nrf_power_resetreas_mask_t enumerator values.
 *
 * @sa nrf_power_resetreas_get
 */
NRF_STATIC_INLINE void nrf_power_resetreas_clear(NRF_POWER_Type * p_reg, uint32_t mask);
#endif // NRF_POWER_HAS_RESETREAS

#if defined(POWER_POWERSTATUS_LTEMODEM_Msk) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for getting power status of the LTE Modem domain.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @retval true  The LTE Modem domain is powered on.
 * @retval false The LTE Modem domain is powered off.
 */
NRF_STATIC_INLINE bool nrf_power_powerstatus_get(NRF_POWER_Type const * p_reg);
#endif

#if defined(POWER_RAMSTATUS_RAMBLOCK0_Msk) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for getting the RAMSTATUS register.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return Value with bits set according to the masks in @ref nrf_power_ramblock_mask_t.
 */
NRF_STATIC_INLINE uint32_t nrf_power_ramstatus_get(NRF_POWER_Type const * p_reg);
#endif // defined(POWER_RAMSTATUS_RAMBLOCK0_Msk) || defined(__NRFX_DOXYGEN__)

#if defined(POWER_SYSTEMOFF_SYSTEMOFF_Enter)
/**
 * @brief Function for going into System OFF mode.
 *
 * This function puts the CPU in System OFF mode.
 * The only way to wake up the CPU is by reset.
 *
 * @note This function never returns.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 */
NRF_STATIC_INLINE void nrf_power_system_off(NRF_POWER_Type * p_reg);
#endif // defined(POWER_SYSTEMOFF_SYSTEMOFF_Enter)

#if NRF_POWER_HAS_POFCON
/**
 * @brief Function for setting the power failure comparator configuration.
 *
 * @param[in] p_reg  Pointer to the structure of registers of the peripheral.
 * @param[in] enable True if the power failure comparator is to be enabled, false otherwise.
 * @param[in] thr    voltage threshold value.
 */
NRF_STATIC_INLINE void nrf_power_pofcon_set(NRF_POWER_Type *    p_reg,
                                            bool                enable,
                                            nrf_power_pof_thr_t thr);

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
NRF_STATIC_INLINE nrf_power_pof_thr_t nrf_power_pofcon_get(NRF_POWER_Type const * p_reg,
                                                           bool *                 p_enabled);
#endif // NRF_POWER_HAS_POFCON

#if NRF_POWER_HAS_POFCON_VDDH
/**
 * @brief Function for setting the VDDH power failure comparator threshold.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] thr   Threshold to be set.
 */
NRF_STATIC_INLINE void nrf_power_pofcon_vddh_set(NRF_POWER_Type *        p_reg,
                                                 nrf_power_pof_thrvddh_t thr);

/**
 * @brief Function for getting the VDDH power failure comparator threshold.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return VDDH threshold currently configured.
 */
NRF_STATIC_INLINE nrf_power_pof_thrvddh_t nrf_power_pofcon_vddh_get(NRF_POWER_Type const * p_reg);
#endif // NRF_POWER_HAS_POFCON_VDDH

/**
 * @brief Function for setting the general purpose retention register.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] val   Value to be set in the register.
 */
NRF_STATIC_INLINE void nrf_power_gpregret_set(NRF_POWER_Type * p_reg, uint8_t val);

/**
 * @brief Function for getting general purpose retention register.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return The value from the register.
 */
NRF_STATIC_INLINE uint8_t nrf_power_gpregret_get(NRF_POWER_Type const * p_reg);

#if defined(POWER_GPREGRET2_GPREGRET_Msk) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for setting the general purpose retention register 2.
 *
 * @note This register is not available in the nRF51 MCU family.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] val   Value to be set in the register.
 */
NRF_STATIC_INLINE void nrf_power_gpregret2_set(NRF_POWER_Type * p_reg, uint8_t val);

/**
 * @brief Function for getting the general purpose retention register 2.
 *
 * @note This register is not available in all MCUs.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return The value from the register.
 */
NRF_STATIC_INLINE uint8_t nrf_power_gpregret2_get(NRF_POWER_Type const * p_reg);
#endif // defined(POWER_GPREGRET2_GPREGRET_Msk) || defined(__NRFX_DOXYGEN__)

/**
 * @brief Function for getting value of the particular general purpose retention register
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] reg_num General purpose retention register number.
 *
 * @return The value from the register
 */
NRF_STATIC_INLINE uint8_t nrf_power_gpregret_ext_get(NRF_POWER_Type const * p_reg, uint8_t reg_num);

/**
 * @brief Function for setting particular general purpose retention register.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] reg_num General purpose retention register number.
 * @param[in] val     Value to be set in the register
 */
NRF_STATIC_INLINE void nrf_power_gpregret_ext_set(NRF_POWER_Type * p_reg,
                                                  uint8_t          reg_num,
                                                  uint8_t          val);

#if NRF_POWER_HAS_DCDCEN
/**
 * @brief Enable or disable DCDC converter
 *
 * @note If the device consist of high voltage power input (VDDH), this setting
 *       will relate to the converter on low voltage side (1.3&nbsp;V output).
 *
 * @param[in] p_reg  Pointer to the structure of registers of the peripheral.
 * @param[in] enable True if DCDC converter is to be enabled, false otherwise.
 */
NRF_STATIC_INLINE void nrf_power_dcdcen_set(NRF_POWER_Type * p_reg, bool enable);

/**
 * @brief Function for getting the state of the DCDC converter.
 *
 * @note If the device consist of high voltage power input (VDDH), this setting
 *       will relate to the converter on low voltage side (1.3&nbsp;V output).
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @retval true  Converter is enabled.
 * @retval false Converter is disabled.
 */
NRF_STATIC_INLINE bool nrf_power_dcdcen_get(NRF_POWER_Type const * p_reg);
#endif // NRF_POWER_HAS_DCDCEN

#if defined(POWER_RAM_POWER_S0POWER_Msk) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Turn ON sections in the selected RAM block.
 *
 * This function turns ON several sections in one block and also block retention.
 *
 * @sa nrf_power_rampower_mask_t
 * @sa nrf_power_rampower_mask_off
 *
 * @param[in] p_reg        Pointer to the structure of registers of the peripheral.
 * @param[in] block        RAM block index.
 * @param[in] section_mask Mask of the sections created by merging
 *                         @ref nrf_power_rampower_mask_t flags.
 */
NRF_STATIC_INLINE void nrf_power_rampower_mask_on(NRF_POWER_Type * p_reg,
                                                  uint8_t          block,
                                                  uint32_t         section_mask);

/**
 * @brief Turn ON sections in the selected RAM block.
 *
 * This function turns OFF several sections in one block and also block retention.
 *
 * @sa nrf_power_rampower_mask_t
 * @sa nrf_power_rampower_mask_off
 *
 * @param[in] p_reg        Pointer to the structure of registers of the peripheral.
 * @param[in] block        RAM block index.
 * @param[in] section_mask Mask of the sections created by merging
 *                         @ref nrf_power_rampower_mask_t flags.
 */
NRF_STATIC_INLINE void nrf_power_rampower_mask_off(NRF_POWER_Type * p_reg,
                                                   uint8_t          block,
                                                   uint32_t         section_mask);

/**
 * @brief Function for getting the ON mask and retention sections in the selected RAM block.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] block RAM block index.
 *
 * @return Mask of sections state composed from @ref nrf_power_rampower_mask_t flags.
 */
NRF_STATIC_INLINE uint32_t nrf_power_rampower_mask_get(NRF_POWER_Type const * p_reg, uint8_t block);
#endif /* defined(POWER_RAM_POWER_S0POWER_Msk) || defined(__NRFX_DOXYGEN__) */

#if NRF_POWER_HAS_DCDCEN_VDDH
/**
 * @brief Function for enabling or disabling the DCDC converter on VDDH.
 *
 * @param[in] p_reg  Pointer to the structure of registers of the peripheral.
 * @param[in] enable True if DCDC converter on VDDH is to be enabled, false otherwise.
 */
NRF_STATIC_INLINE void nrf_power_dcdcen_vddh_set(NRF_POWER_Type * p_reg, bool enable);

/**
 * @brief Function for getting the state of DCDC converter on VDDH.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @retval true  Converter is enabled.
 * @retval false Converter is disabled.
 */
NRF_STATIC_INLINE bool nrf_power_dcdcen_vddh_get(NRF_POWER_Type const * p_reg);
#endif // NRF_POWER_HAS_DCDCEN_VDDH

#if NRF_POWER_HAS_MAINREGSTATUS
/**
 * @brief Function for getting the main supply status.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return The current main supply status.
 */
NRF_STATIC_INLINE
nrf_power_mainregstatus_t nrf_power_mainregstatus_get(NRF_POWER_Type const * p_reg);
#endif // NRF_POWER_HAS_MAINREGSTATUS

#if NRF_POWER_HAS_USBREG
/**
 * @brief Function for getting the whole USBREGSTATUS register.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return The USBREGSTATUS register value.
 *         Use @ref nrf_power_usbregstatus_mask_t values for bit masking.
 *
 * @sa nrf_power_usbregstatus_vbusdet_get
 * @sa nrf_power_usbregstatus_outrdy_get
 */
NRF_STATIC_INLINE uint32_t nrf_power_usbregstatus_get(NRF_POWER_Type const * p_reg);

/**
 * @brief Function for getting the VBUS input detection status.
 *
 * USBDETECTED and USBREMOVED events are derived from this information
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @retval false VBUS voltage below valid threshold.
 * @retval true  VBUS voltage above valid threshold.
 *
 * @sa nrf_power_usbregstatus_get
 */
NRF_STATIC_INLINE bool nrf_power_usbregstatus_vbusdet_get(NRF_POWER_Type const * p_reg);

/**
 * @brief Function for getting the state of the elapsed time for the USB supply output settling.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @retval false USBREG output settling time not elapsed.
 * @retval true  USBREG output settling time elapsed
 *               (same information as USBPWRRDY event).
 *
 * @sa nrf_power_usbregstatus_get
 */
NRF_STATIC_INLINE bool nrf_power_usbregstatus_outrdy_get(NRF_POWER_Type const * p_reg);
#endif // NRF_POWER_HAS_USBREG

#ifndef NRF_DECLARE_ONLY

NRF_STATIC_INLINE void nrf_power_task_trigger(NRF_POWER_Type * p_reg, nrf_power_task_t task)
{
    *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)task)) = 0x1UL;
}

NRF_STATIC_INLINE uint32_t nrf_power_task_address_get(NRF_POWER_Type const * p_reg,
                                                      nrf_power_task_t       task)
{
    return ((uint32_t)p_reg + (uint32_t)task);
}

NRF_STATIC_INLINE void nrf_power_event_clear(NRF_POWER_Type * p_reg, nrf_power_event_t event)
{
    *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)event)) = 0x0UL;
    nrf_event_readback((uint8_t *)p_reg + (uint32_t)event);
}

NRF_STATIC_INLINE bool nrf_power_event_check(NRF_POWER_Type const * p_reg, nrf_power_event_t event)
{
    return (bool)*(volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)event);
}

NRF_STATIC_INLINE bool nrf_power_event_get_and_clear(NRF_POWER_Type *  p_reg,
                                                     nrf_power_event_t event)
{
    bool ret = nrf_power_event_check(p_reg, event);
    if (ret)
    {
        nrf_power_event_clear(p_reg, event);
    }
    return ret;
}

NRF_STATIC_INLINE uint32_t nrf_power_event_address_get(NRF_POWER_Type const * p_reg,
                                                       nrf_power_event_t      event)
{
    return ((uint32_t)p_reg + (uint32_t)event);
}

NRF_STATIC_INLINE void nrf_power_int_enable(NRF_POWER_Type * p_reg, uint32_t mask)
{
    p_reg->INTENSET = mask;
}

NRF_STATIC_INLINE uint32_t nrf_power_int_enable_check(NRF_POWER_Type const * p_reg, uint32_t mask)
{
    return p_reg->INTENSET & mask;
}

NRF_STATIC_INLINE uint32_t nrf_power_int_enable_get(NRF_POWER_Type const * p_reg)
{
    return p_reg->INTENSET;
}

NRF_STATIC_INLINE void nrf_power_int_disable(NRF_POWER_Type * p_reg, uint32_t mask)
{
    p_reg->INTENCLR = mask;
}

#if defined(DPPI_PRESENT)
NRF_STATIC_INLINE void nrf_power_subscribe_set(NRF_POWER_Type * p_reg,
                                               nrf_power_task_t task,
                                               uint8_t          channel)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) task + 0x80uL)) =
            ((uint32_t)channel | POWER_SUBSCRIBE_CONSTLAT_EN_Msk);
}

NRF_STATIC_INLINE void nrf_power_subscribe_clear(NRF_POWER_Type * p_reg, nrf_power_task_t task)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) task + 0x80uL)) = 0;
}

NRF_STATIC_INLINE void nrf_power_publish_set(NRF_POWER_Type *  p_reg,
                                             nrf_power_event_t event,
                                             uint8_t           channel)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) event + 0x80uL)) =
            ((uint32_t)channel | POWER_PUBLISH_SLEEPENTER_EN_Msk);
}

NRF_STATIC_INLINE void nrf_power_publish_clear(NRF_POWER_Type * p_reg, nrf_power_event_t event)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) event + 0x80uL)) = 0;
}
#endif // defined(DPPI_PRESENT)

#if NRF_POWER_HAS_RESETREAS
NRF_STATIC_INLINE uint32_t nrf_power_resetreas_get(NRF_POWER_Type const * p_reg)
{
    return p_reg->RESETREAS;
}

NRF_STATIC_INLINE void nrf_power_resetreas_clear(NRF_POWER_Type * p_reg, uint32_t mask)
{
    p_reg->RESETREAS = mask;
}
#endif // NRF_POWER_HAS_RESETREAS

#if defined(POWER_POWERSTATUS_LTEMODEM_Msk)
NRF_STATIC_INLINE bool nrf_power_powerstatus_get(NRF_POWER_Type const * p_reg)
{
    return (p_reg->POWERSTATUS & POWER_POWERSTATUS_LTEMODEM_Msk) ==
           (POWER_POWERSTATUS_LTEMODEM_ON << POWER_POWERSTATUS_LTEMODEM_Pos);
}
#endif // (POWER_POWERSTATUS_LTEMODEM_Msk)

#if defined(POWER_RAMSTATUS_RAMBLOCK0_Msk)
NRF_STATIC_INLINE uint32_t nrf_power_ramstatus_get(NRF_POWER_Type const * p_reg)
{
    return p_reg->RAMSTATUS;
}
#endif // defined(POWER_RAMSTATUS_RAMBLOCK0_Msk)

#if defined(POWER_SYSTEMOFF_SYSTEMOFF_Enter)
NRF_STATIC_INLINE void nrf_power_system_off(NRF_POWER_Type * p_reg)
{
    p_reg->SYSTEMOFF = POWER_SYSTEMOFF_SYSTEMOFF_Enter;
    __DSB();

    /* Solution for simulated System OFF in debug mode */
    while (true)
    {
        __WFE();
    }
}
#endif // defined(POWER_SYSTEMOFF_SYSTEMOFF_Enter)

#if NRF_POWER_HAS_POFCON
NRF_STATIC_INLINE void nrf_power_pofcon_set(NRF_POWER_Type *    p_reg,
                                            bool                enable,
                                            nrf_power_pof_thr_t thr)
{
    NRFX_ASSERT(thr == (thr & (POWER_POFCON_THRESHOLD_Msk >> POWER_POFCON_THRESHOLD_Pos)));
#if NRF_POWER_HAS_POFCON_VDDH
    uint32_t pofcon = p_reg->POFCON;
    pofcon &= ~(POWER_POFCON_THRESHOLD_Msk | POWER_POFCON_POF_Msk);
    pofcon |=
#else // NRF_POWER_HAS_POFCON_VDDH
    p_reg->POFCON =
#endif
        (((uint32_t)thr) << POWER_POFCON_THRESHOLD_Pos) |
        (enable ?
        (POWER_POFCON_POF_Enabled << POWER_POFCON_POF_Pos)
        :
        (POWER_POFCON_POF_Disabled << POWER_POFCON_POF_Pos));
#if NRF_POWER_HAS_POFCON_VDDH
    p_reg->POFCON = pofcon;
#endif
}

NRF_STATIC_INLINE nrf_power_pof_thr_t nrf_power_pofcon_get(NRF_POWER_Type const * p_reg,
                                                           bool *                 p_enabled)
{
    uint32_t pofcon = p_reg->POFCON;
    if (NULL != p_enabled)
    {
        (*p_enabled) = ((pofcon & POWER_POFCON_POF_Msk) >> POWER_POFCON_POF_Pos)
            == POWER_POFCON_POF_Enabled;
    }
    return (nrf_power_pof_thr_t)((pofcon & POWER_POFCON_THRESHOLD_Msk) >>
        POWER_POFCON_THRESHOLD_Pos);
}
#endif // NRF_POWER_HAS_POFCON

#if NRF_POWER_HAS_POFCON_VDDH
NRF_STATIC_INLINE void nrf_power_pofcon_vddh_set(NRF_POWER_Type *        p_reg,
                                                 nrf_power_pof_thrvddh_t thr)
{
    NRFX_ASSERT(thr == (thr & (POWER_POFCON_THRESHOLDVDDH_Msk >> POWER_POFCON_THRESHOLDVDDH_Pos)));
    uint32_t pofcon = p_reg->POFCON;
    pofcon &= ~POWER_POFCON_THRESHOLDVDDH_Msk;
    pofcon |= (((uint32_t)thr) << POWER_POFCON_THRESHOLDVDDH_Pos);
    p_reg->POFCON = pofcon;
}

NRF_STATIC_INLINE nrf_power_pof_thrvddh_t nrf_power_pofcon_vddh_get(NRF_POWER_Type const * p_reg)
{
    return (nrf_power_pof_thrvddh_t)((p_reg->POFCON & POWER_POFCON_THRESHOLDVDDH_Msk) >>
                                     POWER_POFCON_THRESHOLDVDDH_Pos);
}
#endif // NRF_POWER_HAS_POFCON_VDDH

NRF_STATIC_INLINE void nrf_power_gpregret_set(NRF_POWER_Type * p_reg, uint8_t val)
{
    volatile uint32_t * p_gpregret;
    if (sizeof(p_reg->GPREGRET) > sizeof(uint32_t))
    {
        p_gpregret = &((volatile uint32_t *)p_reg->GPREGRET)[0];
    }
    else
    {
        p_gpregret = &((volatile uint32_t *)&p_reg->GPREGRET)[0];
    }
    *p_gpregret = val;
}

NRF_STATIC_INLINE uint8_t nrf_power_gpregret_get(NRF_POWER_Type const * p_reg)
{
    volatile uint32_t * p_gpregret;
    if (sizeof(p_reg->GPREGRET) > sizeof(uint32_t))
    {
        p_gpregret = &((volatile uint32_t *)p_reg->GPREGRET)[0];
    }
    else
    {
        p_gpregret = &((volatile uint32_t *)&p_reg->GPREGRET)[0];
    }
    return *p_gpregret;
}

NRF_STATIC_INLINE void nrf_power_gpregret_ext_set(NRF_POWER_Type * p_reg,
                                                  uint8_t          reg_num,
                                                  uint8_t          val)
{
#if defined(NRF91_SERIES) || defined(NRF5340_XXAA_APPLICATION) || defined(NRF5340_XXAA_NETWORK)
    p_reg->GPREGRET[reg_num] = val;
#else
    NRFX_ASSERT(reg_num < 1);
    p_reg->GPREGRET = val;
#endif
}

NRF_STATIC_INLINE uint8_t nrf_power_gpregret_ext_get(NRF_POWER_Type const * p_reg, uint8_t reg_num)
{
#if defined(NRF91_SERIES) || defined(NRF5340_XXAA_APPLICATION) || defined(NRF5340_XXAA_NETWORK)
    return p_reg->GPREGRET[reg_num];
#else
    NRFX_ASSERT(reg_num < 1);
    return p_reg->GPREGRET;
#endif
}

#if defined(POWER_GPREGRET2_GPREGRET_Msk)
NRF_STATIC_INLINE void nrf_power_gpregret2_set(NRF_POWER_Type * p_reg, uint8_t val)
{
    p_reg->GPREGRET2 = val;
}

NRF_STATIC_INLINE uint8_t nrf_power_gpregret2_get(NRF_POWER_Type const * p_reg)
{
    return p_reg->GPREGRET2;
}
#endif

#if NRF_POWER_HAS_DCDCEN
NRF_STATIC_INLINE void nrf_power_dcdcen_set(NRF_POWER_Type * p_reg, bool enable)
{
    p_reg->DCDCEN = (enable ? POWER_DCDCEN_DCDCEN_Enabled : POWER_DCDCEN_DCDCEN_Disabled) <<
                    POWER_DCDCEN_DCDCEN_Pos;
}

NRF_STATIC_INLINE bool nrf_power_dcdcen_get(NRF_POWER_Type const * p_reg)
{
    return (p_reg->DCDCEN & POWER_DCDCEN_DCDCEN_Msk)
            ==
           (POWER_DCDCEN_DCDCEN_Enabled << POWER_DCDCEN_DCDCEN_Pos);
}
#endif // NRF_POWER_HAS_DCDCEN

#if defined(POWER_RAM_POWER_S0POWER_Msk)
NRF_STATIC_INLINE void nrf_power_rampower_mask_on(NRF_POWER_Type * p_reg,
                                                  uint8_t          block,
                                                  uint32_t         section_mask)
{
    p_reg->RAM[block].POWERSET = section_mask;
}

NRF_STATIC_INLINE void nrf_power_rampower_mask_off(NRF_POWER_Type * p_reg,
                                                   uint8_t          block,
                                                   uint32_t         section_mask)
{
    p_reg->RAM[block].POWERCLR = section_mask;
}

NRF_STATIC_INLINE uint32_t nrf_power_rampower_mask_get(NRF_POWER_Type const * p_reg, uint8_t block)
{
    return p_reg->RAM[block].POWER;
}
#endif // defined(POWER_RAM_POWER_S0POWER_Msk)

#if NRF_POWER_HAS_DCDCEN_VDDH
NRF_STATIC_INLINE void nrf_power_dcdcen_vddh_set(NRF_POWER_Type * p_reg, bool enable)
{
    if (nrf52_errata_197())
    {
        // Workaround for anomaly 197 "POWER: DCDC of REG0 not functional".
        *(volatile uint32_t *)0x40000638ul = 1ul;
    }
    p_reg->DCDCEN0 = (enable ? POWER_DCDCEN0_DCDCEN_Enabled : POWER_DCDCEN0_DCDCEN_Disabled) <<
                     POWER_DCDCEN0_DCDCEN_Pos;
}

NRF_STATIC_INLINE bool nrf_power_dcdcen_vddh_get(NRF_POWER_Type const * p_reg)
{
    return (p_reg->DCDCEN0 & POWER_DCDCEN0_DCDCEN_Msk)
            ==
           (POWER_DCDCEN0_DCDCEN_Enabled << POWER_DCDCEN0_DCDCEN_Pos);
}
#endif // NRF_POWER_HAS_DCDCEN_VDDH

#if NRF_POWER_HAS_MAINREGSTATUS
NRF_STATIC_INLINE
nrf_power_mainregstatus_t nrf_power_mainregstatus_get(NRF_POWER_Type const * p_reg)
{
    return (nrf_power_mainregstatus_t)(((p_reg->MAINREGSTATUS) &
        POWER_MAINREGSTATUS_MAINREGSTATUS_Msk) >>
        POWER_MAINREGSTATUS_MAINREGSTATUS_Pos);
}
#endif // NRF_POWER_HAS_MAINREGSTATUS

#if NRF_POWER_HAS_USBREG
NRF_STATIC_INLINE uint32_t nrf_power_usbregstatus_get(NRF_POWER_Type const * p_reg)
{
    return p_reg->USBREGSTATUS;
}

NRF_STATIC_INLINE bool nrf_power_usbregstatus_vbusdet_get(NRF_POWER_Type const * p_reg)
{
    return (nrf_power_usbregstatus_get(p_reg) & NRF_POWER_USBREGSTATUS_VBUSDETECT_MASK) != 0;
}

NRF_STATIC_INLINE bool nrf_power_usbregstatus_outrdy_get(NRF_POWER_Type const * p_reg)
{
    return (nrf_power_usbregstatus_get(p_reg) & NRF_POWER_USBREGSTATUS_OUTPUTRDY_MASK) != 0;
}
#endif // NRF_POWER_HAS_USBREG

#endif // NRF_DECLARE_ONLY

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRF_POWER_H__
