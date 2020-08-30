#ifndef NRF51_ERRATAS_H
#define NRF51_ERRATAS_H

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

static bool nrf51_errata_1(void) __UNUSED;
static bool nrf51_errata_2(void) __UNUSED;
static bool nrf51_errata_3(void) __UNUSED;
static bool nrf51_errata_4(void) __UNUSED;
static bool nrf51_errata_5(void) __UNUSED;
static bool nrf51_errata_6(void) __UNUSED;
static bool nrf51_errata_7(void) __UNUSED;
static bool nrf51_errata_8(void) __UNUSED;
static bool nrf51_errata_9(void) __UNUSED;
static bool nrf51_errata_10(void) __UNUSED;
static bool nrf51_errata_11(void) __UNUSED;
static bool nrf51_errata_12(void) __UNUSED;
static bool nrf51_errata_13(void) __UNUSED;
static bool nrf51_errata_14(void) __UNUSED;
static bool nrf51_errata_15(void) __UNUSED;
static bool nrf51_errata_16(void) __UNUSED;
static bool nrf51_errata_17(void) __UNUSED;
static bool nrf51_errata_18(void) __UNUSED;
static bool nrf51_errata_19(void) __UNUSED;
static bool nrf51_errata_20(void) __UNUSED;
static bool nrf51_errata_21(void) __UNUSED;
static bool nrf51_errata_22(void) __UNUSED;
static bool nrf51_errata_23(void) __UNUSED;
static bool nrf51_errata_24(void) __UNUSED;
static bool nrf51_errata_25(void) __UNUSED;
static bool nrf51_errata_26(void) __UNUSED;
static bool nrf51_errata_27(void) __UNUSED;
static bool nrf51_errata_28(void) __UNUSED;
static bool nrf51_errata_29(void) __UNUSED;
static bool nrf51_errata_30(void) __UNUSED;
static bool nrf51_errata_31(void) __UNUSED;
static bool nrf51_errata_32(void) __UNUSED;
static bool nrf51_errata_33(void) __UNUSED;
static bool nrf51_errata_34(void) __UNUSED;
static bool nrf51_errata_35(void) __UNUSED;
static bool nrf51_errata_36(void) __UNUSED;
static bool nrf51_errata_37(void) __UNUSED;
static bool nrf51_errata_38(void) __UNUSED;
static bool nrf51_errata_39(void) __UNUSED;
static bool nrf51_errata_40(void) __UNUSED;
static bool nrf51_errata_41(void) __UNUSED;
static bool nrf51_errata_42(void) __UNUSED;
static bool nrf51_errata_43(void) __UNUSED;
static bool nrf51_errata_44(void) __UNUSED;
static bool nrf51_errata_45(void) __UNUSED;
static bool nrf51_errata_46(void) __UNUSED;
static bool nrf51_errata_47(void) __UNUSED;
static bool nrf51_errata_48(void) __UNUSED;
static bool nrf51_errata_49(void) __UNUSED;
static bool nrf51_errata_50(void) __UNUSED;
static bool nrf51_errata_51(void) __UNUSED;
static bool nrf51_errata_52(void) __UNUSED;
static bool nrf51_errata_53(void) __UNUSED;
static bool nrf51_errata_54(void) __UNUSED;
static bool nrf51_errata_55(void) __UNUSED;
static bool nrf51_errata_56(void) __UNUSED;
static bool nrf51_errata_57(void) __UNUSED;
static bool nrf51_errata_58(void) __UNUSED;
static bool nrf51_errata_59(void) __UNUSED;
static bool nrf51_errata_60(void) __UNUSED;
static bool nrf51_errata_61(void) __UNUSED;
static bool nrf51_errata_62(void) __UNUSED;
static bool nrf51_errata_63(void) __UNUSED;
static bool nrf51_errata_64(void) __UNUSED;
static bool nrf51_errata_65(void) __UNUSED;
static bool nrf51_errata_66(void) __UNUSED;
static bool nrf51_errata_67(void) __UNUSED;
static bool nrf51_errata_68(void) __UNUSED;
static bool nrf51_errata_69(void) __UNUSED;
static bool nrf51_errata_70(void) __UNUSED;
static bool nrf51_errata_71(void) __UNUSED;
static bool nrf51_errata_72(void) __UNUSED;
static bool nrf51_errata_73(void) __UNUSED;
static bool nrf51_errata_74(void) __UNUSED;
static bool nrf51_errata_75(void) __UNUSED;
static bool nrf51_errata_76(void) __UNUSED;
static bool nrf51_errata_77(void) __UNUSED;
static bool nrf51_errata_78(void) __UNUSED;

static bool nrf51_errata_1(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_2(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_3(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_4(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        return false;
    #endif
}

static bool nrf51_errata_5(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        return false;
    #endif
}

static bool nrf51_errata_6(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_7(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_8(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_9(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_10(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_11(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_12(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_13(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_14(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_15(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_16(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_17(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_18(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_19(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_20(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_21(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_22(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_23(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_24(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_25(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_26(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_27(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_28(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_29(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_30(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_31(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_32(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_33(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_34(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_35(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return true;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return true;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_36(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_37(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_38(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return true;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return true;
                    case 0x07ul:
                        return true;
                    case 0x08ul:
                        return true;
                    case 0x09ul:
                        return true;
                    case 0x0Aul:
                        return true;
                    case 0x0Bul:
                        return true;
                    case 0x0Cul:
                        return true;
                    case 0x0Dul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_39(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return true;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return true;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_40(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return true;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return true;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_41(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return true;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return true;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_42(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return true;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return true;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_43(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return false;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return true;
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return true;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_44(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return true;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return true;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_45(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return true;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return true;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_46(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return false;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return true;
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return true;
                    case 0x07ul:
                        return true;
                    case 0x08ul:
                        return true;
                    case 0x09ul:
                        return true;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return true;
                    case 0x0Cul:
                        return true;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_47(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return true;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return true;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_48(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return true;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return true;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_49(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return false;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return true;
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return true;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_50(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        return false;
    #endif
}

static bool nrf51_errata_51(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        return false;
    #endif
}

static bool nrf51_errata_52(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        return false;
    #endif
}

static bool nrf51_errata_53(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        return false;
    #endif
}

static bool nrf51_errata_54(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        return false;
    #endif
}

static bool nrf51_errata_55(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_56(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return true;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return true;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_57(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return true;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return true;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_58(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return false;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return true;
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return true;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_59(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return false;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return true;
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return true;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_60(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return false;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return true;
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return true;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_61(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return false;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return true;
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return true;
                    case 0x07ul:
                        return true;
                    case 0x08ul:
                        return true;
                    case 0x09ul:
                        return true;
                    case 0x0Aul:
                        return true;
                    case 0x0Bul:
                        return true;
                    case 0x0Cul:
                        return true;
                    case 0x0Dul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_62(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return false;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return true;
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return true;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_63(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return true;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return true;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_64(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return true;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return true;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_65(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return true;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return true;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_66(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return false;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return true;
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return true;
                    case 0x07ul:
                        return true;
                    case 0x08ul:
                        return true;
                    case 0x09ul:
                        return true;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return true;
                    case 0x0Cul:
                        return true;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_67(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return false;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return true;
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return true;
                    case 0x07ul:
                        return true;
                    case 0x08ul:
                        return true;
                    case 0x09ul:
                        return true;
                    case 0x0Aul:
                        return true;
                    case 0x0Bul:
                        return true;
                    case 0x0Cul:
                        return true;
                    case 0x0Dul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_68(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return false;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return true;
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return true;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_69(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return false;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return false;
                    case 0x07ul:
                        return true;
                    case 0x08ul:
                        return true;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return true;
                    case 0x0Cul:
                        return true;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_70(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return false;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return true;
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return true;
                    case 0x07ul:
                        return true;
                    case 0x08ul:
                        return true;
                    case 0x09ul:
                        return true;
                    case 0x0Aul:
                        return true;
                    case 0x0Bul:
                        return true;
                    case 0x0Cul:
                        return true;
                    case 0x0Dul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_71(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return false;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return false;
                    case 0x07ul:
                        return true;
                    case 0x08ul:
                        return true;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return true;
                    case 0x0Cul:
                        return true;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_72(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return true;
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return true;
                    case 0x07ul:
                        return true;
                    case 0x08ul:
                        return true;
                    case 0x09ul:
                        return true;
                    case 0x0Aul:
                        return true;
                    case 0x0Bul:
                        return true;
                    case 0x0Cul:
                        return true;
                    case 0x0Dul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_73(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return false;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return false;
                    case 0x07ul:
                        return true;
                    case 0x08ul:
                        return true;
                    case 0x09ul:
                        return true;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return true;
                    case 0x0Cul:
                        return true;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_74(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return false;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return true;
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return true;
                    case 0x07ul:
                        return true;
                    case 0x08ul:
                        return true;
                    case 0x09ul:
                        return true;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return true;
                    case 0x0Cul:
                        return true;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_75(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return false;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return true;
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return true;
                    case 0x07ul:
                        return true;
                    case 0x08ul:
                        return true;
                    case 0x09ul:
                        return true;
                    case 0x0Aul:
                        return true;
                    case 0x0Bul:
                        return true;
                    case 0x0Cul:
                        return true;
                    case 0x0Dul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_76(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return false;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return false;
                    case 0x07ul:
                        return false;
                    case 0x08ul:
                        return false;
                    case 0x09ul:
                        return false;
                    case 0x0Aul:
                        return true;
                    case 0x0Bul:
                        return false;
                    case 0x0Cul:
                        return false;
                    case 0x0Dul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf51_errata_77(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        return false;
    #endif
}

static bool nrf51_errata_78(void)
{
    #ifndef NRF51_SERIES
        return false;
    #else
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF51422_XXAA) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAB) || defined (DEVELOP_IN_NRF51422)\
         || defined (NRF51422_XXAC) || defined (DEVELOP_IN_NRF51422)
            if (var1 == 0x01)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return false;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return true;
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return true;
                    case 0x07ul:
                        return true;
                    case 0x08ul:
                        return true;
                    case 0x09ul:
                        return true;
                    case 0x0Aul:
                        return false;
                    case 0x0Bul:
                        return true;
                    case 0x0Cul:
                        return true;
                    case 0x0Dul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

#endif /* NRF51_ERRATAS_H */
