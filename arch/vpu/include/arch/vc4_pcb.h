/*=============================================================================
Copyright (C) 2016-2017 Authors of rpi-open-firmware
All rights reserved.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

FILE DESCRIPTION
Process control block.

=============================================================================*/

#pragma once

typedef struct {
        uint32_t r23; // 0
        uint32_t r22;
        uint32_t r21;
        uint32_t r20;
        uint32_t r19;
        uint32_t r18;
        uint32_t r17;
        uint32_t r16;
        uint32_t r15;
        uint32_t r14;
        uint32_t r13;
        uint32_t r12;
        uint32_t r11;
        uint32_t r10;
        uint32_t r9;
        uint32_t r8;
        uint32_t r7;
        uint32_t r6; // +68

        uint32_t r5;
        uint32_t r4;
        uint32_t r3;
        uint32_t r2;
        uint32_t r1;
        uint32_t r0; // +92

        uint32_t lr; // +96

        uint32_t sr; // +100
        uint32_t pc; // +104
} vc4_saved_state_t;

