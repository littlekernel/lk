#ifndef NRF91_ERRATAS_H
#define NRF91_ERRATAS_H

/*

Copyright (c) 2010 - 2020, Nordic Semiconductor ASA All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

3. Neither the name of Nordic Semiconductor ASA nor the names of its
   contributors may be used to endorse or promote products derived from this
   software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

*/

#include <stdint.h>
#include <stdbool.h>
#include "compiler_abstraction.h"

static bool nrf91_errata_1(void) __UNUSED;
static bool nrf91_errata_2(void) __UNUSED;
static bool nrf91_errata_4(void) __UNUSED;
static bool nrf91_errata_6(void) __UNUSED;
static bool nrf91_errata_7(void) __UNUSED;
static bool nrf91_errata_8(void) __UNUSED;
static bool nrf91_errata_9(void) __UNUSED;
static bool nrf91_errata_10(void) __UNUSED;
static bool nrf91_errata_12(void) __UNUSED;
static bool nrf91_errata_14(void) __UNUSED;
static bool nrf91_errata_15(void) __UNUSED;
static bool nrf91_errata_16(void) __UNUSED;
static bool nrf91_errata_17(void) __UNUSED;
static bool nrf91_errata_20(void) __UNUSED;
static bool nrf91_errata_21(void) __UNUSED;
static bool nrf91_errata_23(void) __UNUSED;
static bool nrf91_errata_24(void) __UNUSED;
static bool nrf91_errata_26(void) __UNUSED;
static bool nrf91_errata_27(void) __UNUSED;
static bool nrf91_errata_28(void) __UNUSED;
static bool nrf91_errata_29(void) __UNUSED;
static bool nrf91_errata_30(void) __UNUSED;
static bool nrf91_errata_31(void) __UNUSED;
static bool nrf91_errata_32(void) __UNUSED;
static bool nrf91_errata_33(void) __UNUSED;

static bool nrf91_errata_1(void)
{
    #ifndef NRF91_SERIES
        return false;
    #else
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            uint32_t var1 = *(uint32_t *)0x00FF0130ul;
            uint32_t var2 = *(uint32_t *)0x00FF0134ul;
        #endif
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            if (var1 == 0x09)
            {
                switch(var2)
                {
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf91_errata_2(void)
{
    #ifndef NRF91_SERIES
        return false;
    #else
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            uint32_t var1 = *(uint32_t *)0x00FF0130ul;
            uint32_t var2 = *(uint32_t *)0x00FF0134ul;
        #endif
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            if (var1 == 0x09)
            {
                switch(var2)
                {
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf91_errata_4(void)
{
    #ifndef NRF91_SERIES
        return false;
    #else
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            uint32_t var1 = *(uint32_t *)0x00FF0130ul;
            uint32_t var2 = *(uint32_t *)0x00FF0134ul;
        #endif
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            if (var1 == 0x09)
            {
                switch(var2)
                {
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf91_errata_6(void)
{
    #ifndef NRF91_SERIES
        return false;
    #else
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            uint32_t var1 = *(uint32_t *)0x00FF0130ul;
            uint32_t var2 = *(uint32_t *)0x00FF0134ul;
        #endif
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            if (var1 == 0x09)
            {
                switch(var2)
                {
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf91_errata_7(void)
{
    #ifndef NRF91_SERIES
        return false;
    #else
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            uint32_t var1 = *(uint32_t *)0x00FF0130ul;
            uint32_t var2 = *(uint32_t *)0x00FF0134ul;
        #endif
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            if (var1 == 0x09)
            {
                switch(var2)
                {
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf91_errata_8(void)
{
    #ifndef NRF91_SERIES
        return false;
    #else
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            uint32_t var1 = *(uint32_t *)0x00FF0130ul;
            uint32_t var2 = *(uint32_t *)0x00FF0134ul;
        #endif
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            if (var1 == 0x09)
            {
                switch(var2)
                {
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf91_errata_9(void)
{
    #ifndef NRF91_SERIES
        return false;
    #else
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            uint32_t var1 = *(uint32_t *)0x00FF0130ul;
            uint32_t var2 = *(uint32_t *)0x00FF0134ul;
        #endif
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            if (var1 == 0x09)
            {
                switch(var2)
                {
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf91_errata_10(void)
{
    #ifndef NRF91_SERIES
        return false;
    #else
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            uint32_t var1 = *(uint32_t *)0x00FF0130ul;
            uint32_t var2 = *(uint32_t *)0x00FF0134ul;
        #endif
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            if (var1 == 0x09)
            {
                switch(var2)
                {
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf91_errata_12(void)
{
    #ifndef NRF91_SERIES
        return false;
    #else
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            uint32_t var1 = *(uint32_t *)0x00FF0130ul;
            uint32_t var2 = *(uint32_t *)0x00FF0134ul;
        #endif
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            if (var1 == 0x09)
            {
                switch(var2)
                {
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf91_errata_14(void)
{
    #ifndef NRF91_SERIES
        return false;
    #else
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            uint32_t var1 = *(uint32_t *)0x00FF0130ul;
            uint32_t var2 = *(uint32_t *)0x00FF0134ul;
        #endif
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            if (var1 == 0x09)
            {
                switch(var2)
                {
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf91_errata_15(void)
{
    #ifndef NRF91_SERIES
        return false;
    #else
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            uint32_t var1 = *(uint32_t *)0x00FF0130ul;
            uint32_t var2 = *(uint32_t *)0x00FF0134ul;
        #endif
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            if (var1 == 0x09)
            {
                switch(var2)
                {
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf91_errata_16(void)
{
    #ifndef NRF91_SERIES
        return false;
    #else
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            uint32_t var1 = *(uint32_t *)0x00FF0130ul;
            uint32_t var2 = *(uint32_t *)0x00FF0134ul;
        #endif
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            if (var1 == 0x09)
            {
                switch(var2)
                {
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf91_errata_17(void)
{
    #ifndef NRF91_SERIES
        return false;
    #else
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            uint32_t var1 = *(uint32_t *)0x00FF0130ul;
            uint32_t var2 = *(uint32_t *)0x00FF0134ul;
        #endif
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            if (var1 == 0x09)
            {
                switch(var2)
                {
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf91_errata_20(void)
{
    #ifndef NRF91_SERIES
        return false;
    #else
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            uint32_t var1 = *(uint32_t *)0x00FF0130ul;
            uint32_t var2 = *(uint32_t *)0x00FF0134ul;
        #endif
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            if (var1 == 0x09)
            {
                switch(var2)
                {
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf91_errata_21(void)
{
    #ifndef NRF91_SERIES
        return false;
    #else
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            uint32_t var1 = *(uint32_t *)0x00FF0130ul;
            uint32_t var2 = *(uint32_t *)0x00FF0134ul;
        #endif
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            if (var1 == 0x09)
            {
                switch(var2)
                {
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf91_errata_23(void)
{
    #ifndef NRF91_SERIES
        return false;
    #else
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            uint32_t var1 = *(uint32_t *)0x00FF0130ul;
            uint32_t var2 = *(uint32_t *)0x00FF0134ul;
        #endif
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            if (var1 == 0x09)
            {
                switch(var2)
                {
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf91_errata_24(void)
{
    #ifndef NRF91_SERIES
        return false;
    #else
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            uint32_t var1 = *(uint32_t *)0x00FF0130ul;
            uint32_t var2 = *(uint32_t *)0x00FF0134ul;
        #endif
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            if (var1 == 0x09)
            {
                switch(var2)
                {
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf91_errata_26(void)
{
    #ifndef NRF91_SERIES
        return false;
    #else
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            uint32_t var1 = *(uint32_t *)0x00FF0130ul;
            uint32_t var2 = *(uint32_t *)0x00FF0134ul;
        #endif
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            if (var1 == 0x09)
            {
                switch(var2)
                {
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf91_errata_27(void)
{
    #ifndef NRF91_SERIES
        return false;
    #else
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            uint32_t var1 = *(uint32_t *)0x00FF0130ul;
            uint32_t var2 = *(uint32_t *)0x00FF0134ul;
        #endif
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            if (var1 == 0x09)
            {
                switch(var2)
                {
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf91_errata_28(void)
{
    #ifndef NRF91_SERIES
        return false;
    #else
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            uint32_t var1 = *(uint32_t *)0x00FF0130ul;
            uint32_t var2 = *(uint32_t *)0x00FF0134ul;
        #endif
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            if (var1 == 0x09)
            {
                switch(var2)
                {
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf91_errata_29(void)
{
    #ifndef NRF91_SERIES
        return false;
    #else
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            uint32_t var1 = *(uint32_t *)0x00FF0130ul;
            uint32_t var2 = *(uint32_t *)0x00FF0134ul;
        #endif
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            if (var1 == 0x09)
            {
                switch(var2)
                {
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf91_errata_30(void)
{
    #ifndef NRF91_SERIES
        return false;
    #else
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            uint32_t var1 = *(uint32_t *)0x00FF0130ul;
            uint32_t var2 = *(uint32_t *)0x00FF0134ul;
        #endif
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            if (var1 == 0x09)
            {
                switch(var2)
                {
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf91_errata_31(void)
{
    #ifndef NRF91_SERIES
        return false;
    #else
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            uint32_t var1 = *(uint32_t *)0x00FF0130ul;
            uint32_t var2 = *(uint32_t *)0x00FF0134ul;
        #endif
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            if (var1 == 0x09)
            {
                switch(var2)
                {
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf91_errata_32(void)
{
    #ifndef NRF91_SERIES
        return false;
    #else
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            uint32_t var1 = *(uint32_t *)0x00FF0130ul;
            uint32_t var2 = *(uint32_t *)0x00FF0134ul;
        #endif
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            if (var1 == 0x09)
            {
                switch(var2)
                {
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf91_errata_33(void)
{
    #ifndef NRF91_SERIES
        return false;
    #else
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            uint32_t var1 = *(uint32_t *)0x00FF0130ul;
            uint32_t var2 = *(uint32_t *)0x00FF0134ul;
        #endif
        #if defined (NRF9160_XXAA) || defined (DEVELOP_IN_NRF9160)
            if (var1 == 0x09)
            {
                switch(var2)
                {
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

#endif /* NRF91_ERRATAS_H */
