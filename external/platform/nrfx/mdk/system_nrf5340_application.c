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
#include "system_nrf5340_application.h"

/*lint ++flb "Enter library region" */


/* NRF5340 application core uses a variable System Clock Frequency that starts at 64MHz */
#define __SYSTEM_CLOCK_MAX      (128000000UL)
#define __SYSTEM_CLOCK_INITIAL  ( 64000000UL)

#define TRACE_PIN_CNF_VALUE (   (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) | \
                                (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos) | \
                                (GPIO_PIN_CNF_PULL_Disabled << GPIO_PIN_CNF_PULL_Pos) | \
                                (GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos) | \
                                (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos) | \
                                (GPIO_PIN_CNF_MCUSEL_TND << GPIO_PIN_CNF_MCUSEL_Pos))

#define TRACE_TRACECLK_PIN   TAD_PSEL_TRACECLK_PIN_Traceclk
#define TRACE_TRACEDATA0_PIN TAD_PSEL_TRACEDATA0_PIN_Tracedata0
#define TRACE_TRACEDATA1_PIN TAD_PSEL_TRACEDATA1_PIN_Tracedata1
#define TRACE_TRACEDATA2_PIN TAD_PSEL_TRACEDATA2_PIN_Tracedata2
#define TRACE_TRACEDATA3_PIN TAD_PSEL_TRACEDATA3_PIN_Tracedata3

#if defined ( __CC_ARM )
    uint32_t SystemCoreClock __attribute__((used)) = __SYSTEM_CLOCK_INITIAL;  
#elif defined ( __ICCARM__ )
    __root uint32_t SystemCoreClock = __SYSTEM_CLOCK_INITIAL;
#elif defined   ( __GNUC__ )
    uint32_t SystemCoreClock __attribute__((used)) = __SYSTEM_CLOCK_INITIAL;
#endif

void SystemCoreClockUpdate(void)
{
#if defined(NRF_TRUSTZONE_NONSECURE)
    SystemCoreClock = __SYSTEM_CLOCK_MAX >> (NRF_CLOCK_NS->HFCLKCTRL & (CLOCK_HFCLKCTRL_HCLK_Msk));
#else
    SystemCoreClock = __SYSTEM_CLOCK_MAX >> (NRF_CLOCK_S->HFCLKCTRL & (CLOCK_HFCLKCTRL_HCLK_Msk));
#endif
}

void SystemInit(void)
{
    #if !defined(NRF_TRUSTZONE_NONSECURE)
        /* Perform Secure-mode initialization routines. */

        /* Set all ARM SAU regions to NonSecure if TrustZone extensions are enabled.
        * Nordic SPU should handle Secure Attribution tasks */
        #if defined (__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3U)
          SAU->CTRL |= (1 << SAU_CTRL_ALLNS_Pos);
        #endif

        /* Workaround for Errata 97 "ERASEPROTECT, APPROTECT, or startup problems" found at the Errata document
           for your device located at https://infocenter.nordicsemi.com/index.jsp  */
        if (nrf53_errata_97())
        {
            if (*((volatile uint32_t *)0x50004A20ul) == 0)
            {
                *((volatile uint32_t *)0x50004A20ul) = 0xDul;
                *((volatile uint32_t *)0x5000491Cul) = 0x1ul;
                *((volatile uint32_t *)0x5000491Cul) = 0x0ul;
            }
        }

        /* Trimming of the device. Copy all the trimming values from FICR into the target addresses. Trim
         until one ADDR is not initialized. */
        uint32_t index = 0;
        for (index = 0; index < 32ul && NRF_FICR_S->TRIMCNF[index].ADDR != (uint32_t *)0xFFFFFFFFul; index++){
            #if defined ( __ICCARM__ )
                /* IAR will complain about the order of volatile pointer accesses. */
                #pragma diag_suppress=Pa082
            #endif
            *NRF_FICR_S->TRIMCNF[index].ADDR = NRF_FICR_S->TRIMCNF[index].DATA;
            #if defined ( __ICCARM__ )
                #pragma diag_default=Pa082
            #endif
        }

        /* errata 64 must be before errata 42, as errata 42 is dependant on the changes in errata 64*/
        /* Workaround for Errata 64 "VREGMAIN has invalid configuration when CPU is running at 128 MHz" found at the Errata document
           for your device located at https://infocenter.nordicsemi.com/index.jsp  */
        if (nrf53_errata_64())
        {
            *((volatile uint32_t *)0x5000470Cul) = 0x29ul;
            *((volatile uint32_t *)0x5000473Cul) = 0x3ul;
        }

        /* Workaround for Errata 42 "Reset value of HFCLKCTRL is invalid" found at the Errata document
           for your device located at https://infocenter.nordicsemi.com/index.jsp  */
        if (nrf53_errata_42())
        {
            *((volatile uint32_t *)0x50039530ul) = 0xBEEF0044ul;
            NRF_CLOCK_S->HFCLKCTRL = CLOCK_HFCLKCTRL_HCLK_Div2 << CLOCK_HFCLKCTRL_HCLK_Pos;
        }

        /* Workaround for Errata 46 "Higher power consumption of LFRC" found at the Errata document
           for your device located at https://infocenter.nordicsemi.com/index.jsp  */
        if (nrf53_errata_46())
        {
            *((volatile uint32_t *)0x5003254Cul) = 0;
        }

        /* Workaround for Errata 49 "SLEEPENTER and SLEEPEXIT events asserted after pin reset" found at the Errata document
           for your device located at https://infocenter.nordicsemi.com/index.jsp  */
        if (nrf53_errata_49())
        {
            if (NRF_RESET_S->RESETREAS & RESET_RESETREAS_RESETPIN_Msk)
            {
                NRF_POWER_S->EVENTS_SLEEPENTER = 0;
                NRF_POWER_S->EVENTS_SLEEPEXIT = 0;
            }
        }

        /* Workaround for Errata 55 "Bits in RESETREAS are set when they should not be" found at the Errata document
           for your device located at https://infocenter.nordicsemi.com/index.jsp  */
        if (nrf53_errata_55())
        {
            if (NRF_RESET_S->RESETREAS & RESET_RESETREAS_RESETPIN_Msk){
                NRF_RESET_S->RESETREAS = ~RESET_RESETREAS_RESETPIN_Msk;
            }
        }

        /* Workaround for Errata 69 "VREGMAIN configuration is not retained in System OFF" found at the Errata document
           for your device located at https://infocenter.nordicsemi.com/index.jsp  */
        if (nrf53_errata_69())
        {
            *((volatile uint32_t *)0x5000470Cul) =0x65ul;
        }
        
        #if defined(CONFIG_NFCT_PINS_AS_GPIOS)

            if ((NRF_UICR_S->NFCPINS & UICR_NFCPINS_PROTECT_Msk) == (UICR_NFCPINS_PROTECT_NFC << UICR_NFCPINS_PROTECT_Pos))
            {
                NRF_NVMC_S->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;

                while (NRF_NVMC_S->READY == NVMC_READY_READY_Busy);
                NRF_UICR_S->NFCPINS &= ~UICR_NFCPINS_PROTECT_Msk;

                while (NRF_NVMC_S->READY == NVMC_READY_READY_Busy);
                NRF_NVMC_S->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;

                while (NRF_NVMC_S->READY == NVMC_READY_READY_Busy);
                NVIC_SystemReset();
            }

        #endif

        /* Enable SWO trace functionality. If ENABLE_SWO is not defined, SWO pin will be used as GPIO (see Product
           Specification to see which one). */
        #if defined (ENABLE_SWO)
            // Enable Trace And Debug peripheral
            NRF_TAD_S->ENABLE = TAD_ENABLE_ENABLE_Msk;
            NRF_TAD_S->CLOCKSTART = TAD_CLOCKSTART_START_Msk;

            // Set up Trace pad SPU firewall
            NRF_SPU_S->GPIOPORT[0].PERM &= ~(1 << TRACE_TRACEDATA0_PIN);

            // Configure trace port pad
            NRF_P0_S->PIN_CNF[TRACE_TRACEDATA0_PIN] = TRACE_PIN_CNF_VALUE;

            // Select trace pin
            NRF_TAD_S->PSEL.TRACEDATA0 = TRACE_TRACEDATA0_PIN;

            // Set trace port speed to 64 MHz
            NRF_TAD_S->TRACEPORTSPEED = TAD_TRACEPORTSPEED_TRACEPORTSPEED_64MHz;
        #endif

        /* Enable Trace functionality. If ENABLE_TRACE is not defined, TRACE pins will be used as GPIOs (see Product
           Specification to see which ones). */
        #if defined (ENABLE_TRACE)
            // Enable Trace And Debug peripheral
            NRF_TAD_S->ENABLE = TAD_ENABLE_ENABLE_Msk;
            NRF_TAD_S->CLOCKSTART = TAD_CLOCKSTART_START_Msk;

            // Set up Trace pads SPU firewall
            NRF_SPU_S->GPIOPORT[0].PERM &= ~(1 << TRACE_TRACECLK_PIN);
            NRF_SPU_S->GPIOPORT[0].PERM &= ~(1 << TRACE_TRACEDATA0_PIN);
            NRF_SPU_S->GPIOPORT[0].PERM &= ~(1 << TRACE_TRACEDATA1_PIN);
            NRF_SPU_S->GPIOPORT[0].PERM &= ~(1 << TRACE_TRACEDATA2_PIN);
            NRF_SPU_S->GPIOPORT[0].PERM &= ~(1 << TRACE_TRACEDATA3_PIN);

            // Configure trace port pads
            NRF_P0_S->PIN_CNF[TRACE_TRACECLK_PIN] =   TRACE_PIN_CNF_VALUE;
            NRF_P0_S->PIN_CNF[TRACE_TRACEDATA0_PIN] = TRACE_PIN_CNF_VALUE;
            NRF_P0_S->PIN_CNF[TRACE_TRACEDATA1_PIN] = TRACE_PIN_CNF_VALUE;
            NRF_P0_S->PIN_CNF[TRACE_TRACEDATA2_PIN] = TRACE_PIN_CNF_VALUE;
            NRF_P0_S->PIN_CNF[TRACE_TRACEDATA3_PIN] = TRACE_PIN_CNF_VALUE;

            // Select trace pins
            NRF_TAD_S->PSEL.TRACECLK   = TRACE_TRACECLK_PIN;
            NRF_TAD_S->PSEL.TRACEDATA0 = TRACE_TRACEDATA0_PIN;
            NRF_TAD_S->PSEL.TRACEDATA1 = TRACE_TRACEDATA1_PIN;
            NRF_TAD_S->PSEL.TRACEDATA2 = TRACE_TRACEDATA2_PIN;
            NRF_TAD_S->PSEL.TRACEDATA3 = TRACE_TRACEDATA3_PIN;

            // Set trace port speed to 64 MHz
            NRF_TAD_S->TRACEPORTSPEED = TAD_TRACEPORTSPEED_TRACEPORTSPEED_64MHz;

        #endif

        /* Allow Non-Secure code to run FPU instructions.
         * If only the secure code should control FPU power state these registers should be configured accordingly in the secure application code. */
        SCB->NSACR |= (3UL << 10);
    #endif

    /* Enable the FPU if the compiler used floating point unit instructions. __FPU_USED is a MACRO defined by the
    * compiler. Since the FPU consumes energy, remember to disable FPU use in the compiler if floating point unit
    * operations are not used in your code. */
    #if (__FPU_USED == 1)
        SCB->CPACR |= (3UL << 20) | (3UL << 22);
        __DSB();
        __ISB();
    #endif

    SystemCoreClockUpdate();
}

/*lint --flb "Leave library region" */
