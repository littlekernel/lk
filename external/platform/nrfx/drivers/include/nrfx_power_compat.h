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

#ifndef NRFX_POWER_COMPAT_H__
#define NRFX_POWER_COMPAT_H__

/**
 * POWER compatibility layer.
 *
 * The following definitions allow using of common code involving the POWER peripheral
 * driver for different SoCs, regardless of whether certain regulator registers
 * are located in the POWER peripheral or in separate peripherals like USBREG and REGULATORS.
 */

#if defined(REGULATORS_PRESENT)
#include <hal/nrf_regulators.h>
#endif

#if NRF_REGULATORS_HAS_POFCON

typedef nrf_regulators_pof_thr_t nrf_power_pof_thr_t;
#define NRF_POWER_POFTHR_V19     NRF_REGULATORS_POFTHR_V19
#define NRF_POWER_POFTHR_V20     NRF_REGULATORS_POFTHR_V20
#define NRF_POWER_POFTHR_V21     NRF_REGULATORS_POFTHR_V21
#define NRF_POWER_POFTHR_V22     NRF_REGULATORS_POFTHR_V22
#define NRF_POWER_POFTHR_V23     NRF_REGULATORS_POFTHR_V23
#define NRF_POWER_POFTHR_V24     NRF_REGULATORS_POFTHR_V24
#define NRF_POWER_POFTHR_V25     NRF_REGULATORS_POFTHR_V25
#define NRF_POWER_POFTHR_V26     NRF_REGULATORS_POFTHR_V26
#define NRF_POWER_POFTHR_V27     NRF_REGULATORS_POFTHR_V27
#define NRF_POWER_POFTHR_V28     NRF_REGULATORS_POFTHR_V28

typedef nrf_regulators_pof_thrvddh_t nrf_power_pof_thrvddh_t;
#define NRF_POWER_POFTHRVDDH_V27     NRF_REGULATORS_POFTHRVDDH_V27
#define NRF_POWER_POFTHRVDDH_V28     NRF_REGULATORS_POFTHRVDDH_V28
#define NRF_POWER_POFTHRVDDH_V29     NRF_REGULATORS_POFTHRVDDH_V29
#define NRF_POWER_POFTHRVDDH_V30     NRF_REGULATORS_POFTHRVDDH_V30
#define NRF_POWER_POFTHRVDDH_V31     NRF_REGULATORS_POFTHRVDDH_V31
#define NRF_POWER_POFTHRVDDH_V32     NRF_REGULATORS_POFTHRVDDH_V32
#define NRF_POWER_POFTHRVDDH_V33     NRF_REGULATORS_POFTHRVDDH_V33
#define NRF_POWER_POFTHRVDDH_V34     NRF_REGULATORS_POFTHRVDDH_V34
#define NRF_POWER_POFTHRVDDH_V35     NRF_REGULATORS_POFTHRVDDH_V35
#define NRF_POWER_POFTHRVDDH_V36     NRF_REGULATORS_POFTHRVDDH_V36
#define NRF_POWER_POFTHRVDDH_V37     NRF_REGULATORS_POFTHRVDDH_V37
#define NRF_POWER_POFTHRVDDH_V38     NRF_REGULATORS_POFTHRVDDH_V38
#define NRF_POWER_POFTHRVDDH_V39     NRF_REGULATORS_POFTHRVDDH_V39
#define NRF_POWER_POFTHRVDDH_V40     NRF_REGULATORS_POFTHRVDDH_V40
#define NRF_POWER_POFTHRVDDH_V41     NRF_REGULATORS_POFTHRVDDH_V41
#define NRF_POWER_POFTHRVDDH_V42     NRF_REGULATORS_POFTHRVDDH_V42

#endif // NRF_REGULATORS_HAS_POFCON

#if defined(USBREG_PRESENT)
#include "nrfx_usbreg.h"

typedef nrfx_usbreg_event_handler_t nrfx_power_usb_event_handler_t;
typedef nrfx_usbreg_config_t        nrfx_power_usbevt_config_t;

typedef nrfx_usbreg_evt_t           nrfx_power_usb_evt_t;
#define NRFX_POWER_USB_EVT_DETECTED NRFX_USBREG_EVT_DETECTED
#define NRFX_POWER_USB_EVT_REMOVED  NRFX_USBREG_EVT_REMOVED
#define NRFX_POWER_USB_EVT_READY    NRFX_USBREG_EVT_READY

typedef nrfx_usbreg_state_t               nrfx_power_usb_state_t;
#define NRFX_POWER_USB_STATE_DISCONNECTED NRFX_USBREG_STATE_DISCONNECTED
#define NRFX_POWER_USB_STATE_CONNECTED    NRFX_USBREG_STATE_CONNECTED
#define NRFX_POWER_USB_STATE_READY        NRFX_USBREG_STATE_READY

#define nrfx_power_usb_handler_get nrfx_usbreg_handler_get
#define nrfx_power_usbevt_init     nrfx_usbreg_init
#define nrfx_power_usbevt_enable   nrfx_usbreg_enable
#define nrfx_power_usbevt_disable  nrfx_usbreg_disable
#define nrfx_power_usbevt_uninit   nrfx_usbreg_uninit
#define nrfx_power_usbstatus_get   nrfx_usbreg_usbstatus_get

#endif // defined(USBREG_PRESENT)

#endif // NRFX_POWER_COMPAT_H__
