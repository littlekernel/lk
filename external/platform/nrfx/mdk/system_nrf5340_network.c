/*

Copyright (c) 2009-2020 ARM Limited. All rights reserved.

    SPDX-License-Identifier: Apache-2.0

Licensed under the Apache License, Version 2.0 (the License); you may
not use this file except in compliance with the License.
You may obtain a copy of the License at

    www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an AS IS BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

NOTICE: This file has been modified by Nordic Semiconductor ASA.

*/

/* NOTE: Template files (including this one) are application specific and therefore expected to
   be copied into the application project folder prior to its use! */

#include <stdint.h>
#include <stdbool.h>
#include "nrf.h"
#include "nrf_erratas.h"
#include "system_nrf5340_network.h"

/*lint ++flb "Enter library region" */


#define __SYSTEM_CLOCK      (64000000UL)     /*!< NRF5340 network core uses a fixed System Clock Frequency of 32MHz */

#if defined ( __CC_ARM )
    uint32_t SystemCoreClock __attribute__((used)) = __SYSTEM_CLOCK;  
#elif defined ( __ICCARM__ )
    __root uint32_t SystemCoreClock = __SYSTEM_CLOCK;
#elif defined   ( __GNUC__ )
    uint32_t SystemCoreClock __attribute__((used)) = __SYSTEM_CLOCK;
#endif

void SystemCoreClockUpdate(void)
{
    SystemCoreClock = __SYSTEM_CLOCK;
}

void SystemInit(void)
{
    /* Trimming of the device. Copy all the trimming values from FICR into the target addresses. Trim
     until one ADDR is not initialized. */
    uint32_t index = 0;
    for (index = 0; index < 32ul && NRF_FICR_NS->TRIMCNF[index].ADDR != (uint32_t *)0xFFFFFFFFul; index++){
        #if defined ( __ICCARM__ )
            /* IAR will complain about the order of volatile pointer accesses. */
            #pragma diag_suppress=Pa082
        #endif
        *NRF_FICR_NS->TRIMCNF[index].ADDR = NRF_FICR_NS->TRIMCNF[index].DATA;
        #if defined ( __ICCARM__ )
            #pragma diag_default=Pa082
        #endif
    }

    /* Workaround for Errata 49 "SLEEPENTER and SLEEPEXIT events asserted after pin reset" found at the Errata document
       for your device located at https://infocenter.nordicsemi.com/index.jsp  */
    if (nrf53_errata_49())
    {
        if (NRF_RESET_NS->RESETREAS & RESET_RESETREAS_RESETPIN_Msk)
        {
            NRF_POWER_NS->EVENTS_SLEEPENTER = 0;
            NRF_POWER_NS->EVENTS_SLEEPEXIT = 0;
        }
    }

    /* Workaround for Errata 55 "Bits in RESETREAS are set when they should not be" found at the Errata document
       for your device located at https://infocenter.nordicsemi.com/index.jsp  */
    if (nrf53_errata_55())
    {
        if (NRF_RESET_NS->RESETREAS & RESET_RESETREAS_RESETPIN_Msk){
            NRF_RESET_NS->RESETREAS = ~RESET_RESETREAS_RESETPIN_Msk;
        }
    }    

    SystemCoreClockUpdate();
}

/*lint --flb "Leave library region" */
