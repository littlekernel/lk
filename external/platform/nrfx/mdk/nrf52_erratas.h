#ifndef NRF52_ERRATAS_H
#define NRF52_ERRATAS_H

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

static bool nrf52_errata_1(void) __UNUSED;
static bool nrf52_errata_2(void) __UNUSED;
static bool nrf52_errata_3(void) __UNUSED;
static bool nrf52_errata_4(void) __UNUSED;
static bool nrf52_errata_7(void) __UNUSED;
static bool nrf52_errata_8(void) __UNUSED;
static bool nrf52_errata_9(void) __UNUSED;
static bool nrf52_errata_10(void) __UNUSED;
static bool nrf52_errata_11(void) __UNUSED;
static bool nrf52_errata_12(void) __UNUSED;
static bool nrf52_errata_15(void) __UNUSED;
static bool nrf52_errata_16(void) __UNUSED;
static bool nrf52_errata_17(void) __UNUSED;
static bool nrf52_errata_20(void) __UNUSED;
static bool nrf52_errata_23(void) __UNUSED;
static bool nrf52_errata_24(void) __UNUSED;
static bool nrf52_errata_25(void) __UNUSED;
static bool nrf52_errata_26(void) __UNUSED;
static bool nrf52_errata_27(void) __UNUSED;
static bool nrf52_errata_28(void) __UNUSED;
static bool nrf52_errata_29(void) __UNUSED;
static bool nrf52_errata_30(void) __UNUSED;
static bool nrf52_errata_31(void) __UNUSED;
static bool nrf52_errata_32(void) __UNUSED;
static bool nrf52_errata_33(void) __UNUSED;
static bool nrf52_errata_34(void) __UNUSED;
static bool nrf52_errata_35(void) __UNUSED;
static bool nrf52_errata_36(void) __UNUSED;
static bool nrf52_errata_37(void) __UNUSED;
static bool nrf52_errata_38(void) __UNUSED;
static bool nrf52_errata_39(void) __UNUSED;
static bool nrf52_errata_40(void) __UNUSED;
static bool nrf52_errata_41(void) __UNUSED;
static bool nrf52_errata_42(void) __UNUSED;
static bool nrf52_errata_43(void) __UNUSED;
static bool nrf52_errata_44(void) __UNUSED;
static bool nrf52_errata_46(void) __UNUSED;
static bool nrf52_errata_47(void) __UNUSED;
static bool nrf52_errata_48(void) __UNUSED;
static bool nrf52_errata_49(void) __UNUSED;
static bool nrf52_errata_51(void) __UNUSED;
static bool nrf52_errata_54(void) __UNUSED;
static bool nrf52_errata_55(void) __UNUSED;
static bool nrf52_errata_57(void) __UNUSED;
static bool nrf52_errata_58(void) __UNUSED;
static bool nrf52_errata_62(void) __UNUSED;
static bool nrf52_errata_63(void) __UNUSED;
static bool nrf52_errata_64(void) __UNUSED;
static bool nrf52_errata_65(void) __UNUSED;
static bool nrf52_errata_66(void) __UNUSED;
static bool nrf52_errata_67(void) __UNUSED;
static bool nrf52_errata_68(void) __UNUSED;
static bool nrf52_errata_70(void) __UNUSED;
static bool nrf52_errata_71(void) __UNUSED;
static bool nrf52_errata_72(void) __UNUSED;
static bool nrf52_errata_73(void) __UNUSED;
static bool nrf52_errata_74(void) __UNUSED;
static bool nrf52_errata_75(void) __UNUSED;
static bool nrf52_errata_76(void) __UNUSED;
static bool nrf52_errata_77(void) __UNUSED;
static bool nrf52_errata_78(void) __UNUSED;
static bool nrf52_errata_79(void) __UNUSED;
static bool nrf52_errata_81(void) __UNUSED;
static bool nrf52_errata_83(void) __UNUSED;
static bool nrf52_errata_84(void) __UNUSED;
static bool nrf52_errata_86(void) __UNUSED;
static bool nrf52_errata_87(void) __UNUSED;
static bool nrf52_errata_88(void) __UNUSED;
static bool nrf52_errata_89(void) __UNUSED;
static bool nrf52_errata_91(void) __UNUSED;
static bool nrf52_errata_94(void) __UNUSED;
static bool nrf52_errata_96(void) __UNUSED;
static bool nrf52_errata_97(void) __UNUSED;
static bool nrf52_errata_98(void) __UNUSED;
static bool nrf52_errata_101(void) __UNUSED;
static bool nrf52_errata_102(void) __UNUSED;
static bool nrf52_errata_103(void) __UNUSED;
static bool nrf52_errata_104(void) __UNUSED;
static bool nrf52_errata_106(void) __UNUSED;
static bool nrf52_errata_107(void) __UNUSED;
static bool nrf52_errata_108(void) __UNUSED;
static bool nrf52_errata_109(void) __UNUSED;
static bool nrf52_errata_110(void) __UNUSED;
static bool nrf52_errata_111(void) __UNUSED;
static bool nrf52_errata_112(void) __UNUSED;
static bool nrf52_errata_113(void) __UNUSED;
static bool nrf52_errata_115(void) __UNUSED;
static bool nrf52_errata_116(void) __UNUSED;
static bool nrf52_errata_117(void) __UNUSED;
static bool nrf52_errata_118(void) __UNUSED;
static bool nrf52_errata_119(void) __UNUSED;
static bool nrf52_errata_120(void) __UNUSED;
static bool nrf52_errata_121(void) __UNUSED;
static bool nrf52_errata_122(void) __UNUSED;
static bool nrf52_errata_127(void) __UNUSED;
static bool nrf52_errata_128(void) __UNUSED;
static bool nrf52_errata_131(void) __UNUSED;
static bool nrf52_errata_132(void) __UNUSED;
static bool nrf52_errata_133(void) __UNUSED;
static bool nrf52_errata_134(void) __UNUSED;
static bool nrf52_errata_135(void) __UNUSED;
static bool nrf52_errata_136(void) __UNUSED;
static bool nrf52_errata_138(void) __UNUSED;
static bool nrf52_errata_140(void) __UNUSED;
static bool nrf52_errata_141(void) __UNUSED;
static bool nrf52_errata_142(void) __UNUSED;
static bool nrf52_errata_143(void) __UNUSED;
static bool nrf52_errata_144(void) __UNUSED;
static bool nrf52_errata_145(void) __UNUSED;
static bool nrf52_errata_146(void) __UNUSED;
static bool nrf52_errata_147(void) __UNUSED;
static bool nrf52_errata_149(void) __UNUSED;
static bool nrf52_errata_150(void) __UNUSED;
static bool nrf52_errata_151(void) __UNUSED;
static bool nrf52_errata_153(void) __UNUSED;
static bool nrf52_errata_154(void) __UNUSED;
static bool nrf52_errata_155(void) __UNUSED;
static bool nrf52_errata_156(void) __UNUSED;
static bool nrf52_errata_158(void) __UNUSED;
static bool nrf52_errata_160(void) __UNUSED;
static bool nrf52_errata_162(void) __UNUSED;
static bool nrf52_errata_163(void) __UNUSED;
static bool nrf52_errata_164(void) __UNUSED;
static bool nrf52_errata_166(void) __UNUSED;
static bool nrf52_errata_170(void) __UNUSED;
static bool nrf52_errata_171(void) __UNUSED;
static bool nrf52_errata_172(void) __UNUSED;
static bool nrf52_errata_173(void) __UNUSED;
static bool nrf52_errata_174(void) __UNUSED;
static bool nrf52_errata_176(void) __UNUSED;
static bool nrf52_errata_178(void) __UNUSED;
static bool nrf52_errata_179(void) __UNUSED;
static bool nrf52_errata_180(void) __UNUSED;
static bool nrf52_errata_181(void) __UNUSED;
static bool nrf52_errata_182(void) __UNUSED;
static bool nrf52_errata_183(void) __UNUSED;
static bool nrf52_errata_184(void) __UNUSED;
static bool nrf52_errata_186(void) __UNUSED;
static bool nrf52_errata_187(void) __UNUSED;
static bool nrf52_errata_189(void) __UNUSED;
static bool nrf52_errata_190(void) __UNUSED;
static bool nrf52_errata_191(void) __UNUSED;
static bool nrf52_errata_192(void) __UNUSED;
static bool nrf52_errata_193(void) __UNUSED;
static bool nrf52_errata_194(void) __UNUSED;
static bool nrf52_errata_195(void) __UNUSED;
static bool nrf52_errata_196(void) __UNUSED;
static bool nrf52_errata_197(void) __UNUSED;
static bool nrf52_errata_198(void) __UNUSED;
static bool nrf52_errata_199(void) __UNUSED;
static bool nrf52_errata_200(void) __UNUSED;
static bool nrf52_errata_201(void) __UNUSED;
static bool nrf52_errata_202(void) __UNUSED;
static bool nrf52_errata_204(void) __UNUSED;
static bool nrf52_errata_208(void) __UNUSED;
static bool nrf52_errata_209(void) __UNUSED;
static bool nrf52_errata_210(void) __UNUSED;
static bool nrf52_errata_211(void) __UNUSED;
static bool nrf52_errata_212(void) __UNUSED;
static bool nrf52_errata_213(void) __UNUSED;
static bool nrf52_errata_214(void) __UNUSED;
static bool nrf52_errata_215(void) __UNUSED;
static bool nrf52_errata_216(void) __UNUSED;
static bool nrf52_errata_217(void) __UNUSED;
static bool nrf52_errata_218(void) __UNUSED;
static bool nrf52_errata_219(void) __UNUSED;
static bool nrf52_errata_220(void) __UNUSED;
static bool nrf52_errata_223(void) __UNUSED;
static bool nrf52_errata_225(void) __UNUSED;
static bool nrf52_errata_228(void) __UNUSED;
static bool nrf52_errata_230(void) __UNUSED;
static bool nrf52_errata_231(void) __UNUSED;
static bool nrf52_errata_232(void) __UNUSED;
static bool nrf52_errata_233(void) __UNUSED;
static bool nrf52_errata_236(void) __UNUSED;
static bool nrf52_errata_237(void) __UNUSED;
static bool nrf52_errata_248(void) __UNUSED;

static bool nrf52_errata_1(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return false;
                    case 0x06ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_2(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return false;
                    case 0x06ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_3(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return false;
                    case 0x06ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_4(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return false;
                    case 0x06ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_7(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return false;
                    case 0x06ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_8(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return false;
                    case 0x06ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_9(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return false;
                    case 0x06ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_10(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return false;
                    case 0x06ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_11(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return false;
                    case 0x06ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_12(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return true;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_15(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #elif defined (NRF52805_XXAA) || defined (DEVELOP_IN_NRF52805)\
         || defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)\
         || defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)\
         || defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return true;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        #if defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)
            if (var1 == 0x0A)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)
            if (var1 == 0x0E)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52805_XXAA) || defined (DEVELOP_IN_NRF52805)
            if (var1 == 0x0F)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_16(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return false;
                    case 0x06ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_17(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return false;
                    case 0x06ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_20(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #elif defined (NRF52805_XXAA) || defined (DEVELOP_IN_NRF52805)\
         || defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)\
         || defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)\
         || defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)\
         || defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)\
         || defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return true;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
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
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)
            if (var1 == 0x0A)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)
            if (var1 == 0x0D)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)
            if (var1 == 0x0E)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52805_XXAA) || defined (DEVELOP_IN_NRF52805)
            if (var1 == 0x0F)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)
            if (var1 == 0x10)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
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

static bool nrf52_errata_23(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return false;
                    case 0x06ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_24(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return false;
                    case 0x06ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_25(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return false;
                    case 0x06ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_26(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return false;
                    case 0x06ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_27(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return false;
                    case 0x06ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_28(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return false;
                    case 0x06ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_29(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return false;
                    case 0x06ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_30(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return false;
                    case 0x06ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_31(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #elif defined (NRF52805_XXAA) || defined (DEVELOP_IN_NRF52805)\
         || defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)\
         || defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return true;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)
            if (var1 == 0x0A)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)
            if (var1 == 0x0E)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52805_XXAA) || defined (DEVELOP_IN_NRF52805)
            if (var1 == 0x0F)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_32(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return false;
                    case 0x06ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_33(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return false;
                    case 0x06ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_34(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return false;
                    case 0x06ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_35(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return false;
                    case 0x06ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_36(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #elif defined (NRF52805_XXAA) || defined (DEVELOP_IN_NRF52805)\
         || defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)\
         || defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)\
         || defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)\
         || defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)\
         || defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return true;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
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
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)
            if (var1 == 0x0A)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)
            if (var1 == 0x0D)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)
            if (var1 == 0x0E)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52805_XXAA) || defined (DEVELOP_IN_NRF52805)
            if (var1 == 0x0F)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)
            if (var1 == 0x10)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
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

static bool nrf52_errata_37(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return false;
                    case 0x06ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_38(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return false;
                    case 0x06ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_39(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return false;
                    case 0x06ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_40(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return false;
                    case 0x06ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_41(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return false;
                    case 0x06ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_42(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return false;
                    case 0x06ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_43(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return false;
                    case 0x06ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_44(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return false;
                    case 0x06ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_46(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return false;
                    case 0x06ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_47(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return false;
                    case 0x06ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_48(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return false;
                    case 0x06ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_49(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return false;
                    case 0x06ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_51(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return true;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_54(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #elif defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return true;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_55(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #elif defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)\
         || defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)\
         || defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return true;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
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
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)
            if (var1 == 0x0D)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)
            if (var1 == 0x10)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
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

static bool nrf52_errata_57(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return false;
                    case 0x06ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_58(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #elif defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return true;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_62(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return false;
                    case 0x06ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_63(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return true;
                    case 0x05ul:
                        return false;
                    case 0x06ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_64(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return true;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_65(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return false;
                    case 0x06ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_66(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #elif defined (NRF52805_XXAA) || defined (DEVELOP_IN_NRF52805)\
         || defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)\
         || defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)\
         || defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)\
         || defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)\
         || defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
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
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)
            if (var1 == 0x0A)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)
            if (var1 == 0x0D)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)
            if (var1 == 0x0E)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52805_XXAA) || defined (DEVELOP_IN_NRF52805)
            if (var1 == 0x0F)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)
            if (var1 == 0x10)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
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

static bool nrf52_errata_67(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return true;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_68(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #elif defined (NRF52805_XXAA) || defined (DEVELOP_IN_NRF52805)\
         || defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)\
         || defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)\
         || defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return true;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        #if defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)
            if (var1 == 0x0A)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)
            if (var1 == 0x0E)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52805_XXAA) || defined (DEVELOP_IN_NRF52805)
            if (var1 == 0x0F)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_70(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return true;
                    case 0x05ul:
                        return false;
                    case 0x06ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_71(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return true;
                    case 0x05ul:
                        return false;
                    case 0x06ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_72(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return true;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_73(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
            uint32_t var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return true;
                    case 0x05ul:
                        return false;
                    case 0x06ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_74(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return true;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_75(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return true;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_76(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return true;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_77(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #elif defined (NRF52805_XXAA) || defined (DEVELOP_IN_NRF52805)\
         || defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)\
         || defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return true;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)
            if (var1 == 0x0A)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)
            if (var1 == 0x0E)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52805_XXAA) || defined (DEVELOP_IN_NRF52805)
            if (var1 == 0x0F)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_78(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #elif defined (NRF52805_XXAA) || defined (DEVELOP_IN_NRF52805)\
         || defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)\
         || defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)\
         || defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)\
         || defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)\
         || defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return true;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
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
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)
            if (var1 == 0x0A)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)
            if (var1 == 0x0D)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)
            if (var1 == 0x0E)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52805_XXAA) || defined (DEVELOP_IN_NRF52805)
            if (var1 == 0x0F)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)
            if (var1 == 0x10)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
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

static bool nrf52_errata_79(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return true;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_81(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #elif defined (NRF52805_XXAA) || defined (DEVELOP_IN_NRF52805)\
         || defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)\
         || defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)\
         || defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return true;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
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
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)
            if (var1 == 0x0A)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)
            if (var1 == 0x0E)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52805_XXAA) || defined (DEVELOP_IN_NRF52805)
            if (var1 == 0x0F)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_83(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #elif defined (NRF52805_XXAA) || defined (DEVELOP_IN_NRF52805)\
         || defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)\
         || defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)\
         || defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return true;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        #if defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)
            if (var1 == 0x0A)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)
            if (var1 == 0x0E)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52805_XXAA) || defined (DEVELOP_IN_NRF52805)
            if (var1 == 0x0F)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_84(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return true;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_86(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return true;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_87(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #elif defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)\
         || defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)\
         || defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return true;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
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
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)
            if (var1 == 0x0D)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)
            if (var1 == 0x10)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
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

static bool nrf52_errata_88(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #elif defined (NRF52805_XXAA) || defined (DEVELOP_IN_NRF52805)\
         || defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)\
         || defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return true;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)
            if (var1 == 0x0A)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)
            if (var1 == 0x0E)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52805_XXAA) || defined (DEVELOP_IN_NRF52805)
            if (var1 == 0x0F)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_89(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #elif defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return true;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_91(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_94(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
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
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_96(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_97(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #elif defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return true;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_98(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_101(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return true;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_102(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return true;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_103(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_104(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_106(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_107(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_108(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return true;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_109(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_110(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_111(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_112(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_113(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #elif defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_115(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_116(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_117(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_118(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_119(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_120(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_121(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_122(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
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
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_127(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_128(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_131(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_132(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_133(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_134(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_135(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_136(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #elif defined (NRF52805_XXAA) || defined (DEVELOP_IN_NRF52805)\
         || defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)\
         || defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)\
         || defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)\
         || defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)\
         || defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return true;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
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
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)
            if (var1 == 0x0A)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)
            if (var1 == 0x0D)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)
            if (var1 == 0x0E)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52805_XXAA) || defined (DEVELOP_IN_NRF52805)
            if (var1 == 0x0F)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)
            if (var1 == 0x10)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
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

static bool nrf52_errata_138(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_140(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_141(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_142(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_143(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #elif defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_144(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_145(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_146(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return true;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_147(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_149(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_150(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #elif defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)\
         || defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        #if defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)
            if (var1 == 0x0A)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_151(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_153(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
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
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_154(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_155(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #elif defined (NRF52805_XXAA) || defined (DEVELOP_IN_NRF52805)\
         || defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)\
         || defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)\
         || defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
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
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)
            if (var1 == 0x0A)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)
            if (var1 == 0x0E)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52805_XXAA) || defined (DEVELOP_IN_NRF52805)
            if (var1 == 0x0F)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_156(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #elif defined (NRF52805_XXAA) || defined (DEVELOP_IN_NRF52805)\
         || defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)\
         || defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)\
         || defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        #if defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)
            if (var1 == 0x0A)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)
            if (var1 == 0x0E)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52805_XXAA) || defined (DEVELOP_IN_NRF52805)
            if (var1 == 0x0F)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_158(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_160(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_162(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_163(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_164(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_166(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
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
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_170(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)\
         || defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)\
         || defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
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
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)
            if (var1 == 0x0D)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)
            if (var1 == 0x10)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
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

static bool nrf52_errata_171(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
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
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_172(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return false;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return true;
                    case 0x03ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_173(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #elif defined (NRF52805_XXAA) || defined (DEVELOP_IN_NRF52805)\
         || defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)\
         || defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)\
         || defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)\
         || defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)\
         || defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
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
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)
            if (var1 == 0x0A)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)
            if (var1 == 0x0D)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)
            if (var1 == 0x0E)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52805_XXAA) || defined (DEVELOP_IN_NRF52805)
            if (var1 == 0x0F)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)
            if (var1 == 0x10)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
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

static bool nrf52_errata_174(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return false;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return true;
                    case 0x03ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_176(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #elif defined (NRF52805_XXAA) || defined (DEVELOP_IN_NRF52805)\
         || defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)\
         || defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)\
         || defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)\
         || defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)\
         || defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
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
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)
            if (var1 == 0x0A)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)
            if (var1 == 0x0D)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)
            if (var1 == 0x0E)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52805_XXAA) || defined (DEVELOP_IN_NRF52805)
            if (var1 == 0x0F)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)
            if (var1 == 0x10)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
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

static bool nrf52_errata_178(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_179(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #elif defined (NRF52805_XXAA) || defined (DEVELOP_IN_NRF52805)\
         || defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)\
         || defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)\
         || defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
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
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)
            if (var1 == 0x0A)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)
            if (var1 == 0x0E)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52805_XXAA) || defined (DEVELOP_IN_NRF52805)
            if (var1 == 0x0F)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_180(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_181(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #elif defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_182(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return false;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_183(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #elif defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)\
         || defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)\
         || defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)\
         || defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)\
         || defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
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
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)
            if (var1 == 0x0A)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)
            if (var1 == 0x0D)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)
            if (var1 == 0x0E)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)
            if (var1 == 0x10)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
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

static bool nrf52_errata_184(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52805_XXAA) || defined (DEVELOP_IN_NRF52805)\
         || defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)\
         || defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)\
         || defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)\
         || defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)\
         || defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
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
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)
            if (var1 == 0x0A)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)
            if (var1 == 0x0D)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)
            if (var1 == 0x0E)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52805_XXAA) || defined (DEVELOP_IN_NRF52805)
            if (var1 == 0x0F)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)
            if (var1 == 0x10)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
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

static bool nrf52_errata_186(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return false;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_187(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)\
         || defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)\
         || defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return false;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return true;
                    case 0x03ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)
            if (var1 == 0x0D)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)
            if (var1 == 0x10)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
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

static bool nrf52_errata_189(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return false;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_190(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)\
         || defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)\
         || defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return false;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return true;
                    case 0x03ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)
            if (var1 == 0x0D)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)
            if (var1 == 0x10)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
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

static bool nrf52_errata_191(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return false;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return true;
                    case 0x03ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_192(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #elif defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)\
         || defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
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
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        #if defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)
            if (var1 == 0x0A)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_193(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return false;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return true;
                    case 0x03ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_194(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #elif defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)\
         || defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)\
         || defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
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
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)
            if (var1 == 0x0D)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)
            if (var1 == 0x10)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
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

static bool nrf52_errata_195(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return false;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return true;
                    case 0x03ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_196(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #elif defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)\
         || defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)\
         || defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
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
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)
            if (var1 == 0x0D)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)
            if (var1 == 0x10)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
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

static bool nrf52_errata_197(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
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
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_198(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return false;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return true;
                    case 0x03ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_199(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
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
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_200(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return false;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_201(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #elif defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)\
         || defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
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
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        #if defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)
            if (var1 == 0x0A)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_202(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return false;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return true;
                    case 0x03ul:
                        return false;
                    default:
                        return false;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_204(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #elif defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)\
         || defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
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
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)
            if (var1 == 0x0A)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_208(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
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
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_209(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
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
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_210(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #elif defined (NRF52805_XXAA) || defined (DEVELOP_IN_NRF52805)\
         || defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)\
         || defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)\
         || defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)\
         || defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)\
         || defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
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
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)
            if (var1 == 0x0A)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)
            if (var1 == 0x0D)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)
            if (var1 == 0x0E)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52805_XXAA) || defined (DEVELOP_IN_NRF52805)
            if (var1 == 0x0F)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)
            if (var1 == 0x10)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
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

static bool nrf52_errata_211(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)\
         || defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)\
         || defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return false;
                    case 0x01ul:
                        return true;
                    case 0x02ul:
                        return true;
                    case 0x03ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)
            if (var1 == 0x0D)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)
            if (var1 == 0x10)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
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

static bool nrf52_errata_212(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #elif defined (NRF52805_XXAA) || defined (DEVELOP_IN_NRF52805)\
         || defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)\
         || defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)\
         || defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)\
         || defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)\
         || defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
                    case 0x02ul:
                        return true;
                    case 0x03ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)
            if (var1 == 0x0A)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)
            if (var1 == 0x0D)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)
            if (var1 == 0x0E)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52805_XXAA) || defined (DEVELOP_IN_NRF52805)
            if (var1 == 0x0F)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)
            if (var1 == 0x10)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
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

static bool nrf52_errata_213(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #elif defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)\
         || defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)\
         || defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return true;
                    case 0x04ul:
                        return true;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
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
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)
            if (var1 == 0x0A)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)
            if (var1 == 0x0E)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_214(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
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
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_215(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
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
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_216(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
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
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_217(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52805_XXAA) || defined (DEVELOP_IN_NRF52805)\
         || defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)\
         || defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)
            if (var1 == 0x0A)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return false;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)
            if (var1 == 0x0E)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52805_XXAA) || defined (DEVELOP_IN_NRF52805)
            if (var1 == 0x0F)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_218(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #elif defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)\
         || defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)\
         || defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
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
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)
            if (var1 == 0x0D)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)
            if (var1 == 0x10)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
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

static bool nrf52_errata_219(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #elif defined (NRF52805_XXAA) || defined (DEVELOP_IN_NRF52805)\
         || defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)\
         || defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)\
         || defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)\
         || defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)\
         || defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
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
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)
            if (var1 == 0x0A)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)
            if (var1 == 0x0D)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)
            if (var1 == 0x0E)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52805_XXAA) || defined (DEVELOP_IN_NRF52805)
            if (var1 == 0x0F)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)
            if (var1 == 0x10)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
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

static bool nrf52_errata_220(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            uint32_t var1;
            uint32_t var2;

            if (*(uint32_t *)0x10000130ul == 0xFFFFFFFF)
            {
                var1 = ((*(uint32_t *)0xF0000FE0ul) & 0x000000FFul);
                var2 = ((*(uint32_t *)0xF0000FE8ul) & 0x000000F0ul) >> 4;
            }
            else
            {
                var1 = *(uint32_t *)0x10000130ul;
                var2 = *(uint32_t *)0x10000134ul;
            }
        #endif
        #if defined (NRF52832_XXAA) || defined (DEVELOP_IN_NRF52832)\
         || defined (NRF52832_XXAB) || defined (DEVELOP_IN_NRF52832)
            if (var1 == 0x06)
            {
                switch(var2)
                {
                    case 0x03ul:
                        return false;
                    case 0x04ul:
                        return false;
                    case 0x05ul:
                        return true;
                    case 0x06ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_223(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)\
         || defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)
            if (var1 == 0x0D)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)
            if (var1 == 0x10)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
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

static bool nrf52_errata_225(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)\
         || defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)
            if (var1 == 0x0D)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)
            if (var1 == 0x10)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
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

static bool nrf52_errata_228(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)\
         || defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)\
         || defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)\
         || defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)\
         || defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
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
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)
            if (var1 == 0x0A)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return false;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)
            if (var1 == 0x0D)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)
            if (var1 == 0x0E)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)
            if (var1 == 0x10)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
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

static bool nrf52_errata_230(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)
            if (var1 == 0x10)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
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

static bool nrf52_errata_231(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)
            if (var1 == 0x10)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return false;
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

static bool nrf52_errata_232(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)\
         || defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)
            if (var1 == 0x0A)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return false;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)
            if (var1 == 0x0E)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        return false;
    #endif
}

static bool nrf52_errata_233(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)\
         || defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)\
         || defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
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
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)
            if (var1 == 0x0D)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)
            if (var1 == 0x10)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
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

static bool nrf52_errata_236(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)\
         || defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)\
         || defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)\
         || defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)\
         || defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
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
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)
            if (var1 == 0x0A)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return false;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)
            if (var1 == 0x0D)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)
            if (var1 == 0x0E)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)
            if (var1 == 0x10)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
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

static bool nrf52_errata_237(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        #if defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)\
         || defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)\
         || defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)\
         || defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)\
         || defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            uint32_t var1 = *(uint32_t *)0x10000130ul;
            uint32_t var2 = *(uint32_t *)0x10000134ul;
        #endif
        #if defined (NRF52840_XXAA) || defined (DEVELOP_IN_NRF52840)
            if (var1 == 0x08)
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
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52810_XXAA) || defined (DEVELOP_IN_NRF52810)
            if (var1 == 0x0A)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52833_XXAA) || defined (DEVELOP_IN_NRF52833)
            if (var1 == 0x0D)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    case 0x01ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52811_XXAA) || defined (DEVELOP_IN_NRF52811)
            if (var1 == 0x0E)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
                    default:
                        return true;
                }
            }
        #endif
        #if defined (NRF52820_XXAA) || defined (DEVELOP_IN_NRF52820)
            if (var1 == 0x10)
            {
                switch(var2)
                {
                    case 0x00ul:
                        return true;
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

static bool nrf52_errata_248(void)
{
    #ifndef NRF52_SERIES
        return false;
    #else
        return false;
    #endif
}

#endif /* NRF52_ERRATAS_H */
