/*
 * Copyright (c) 2014 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#pragma once

typedef enum IRQn {
    NonMaskableInt_IRQn         = -14,    /* 2 Non Maskable Interrupt                */
    MemoryManagement_IRQn       = -12,    /* 4 Cortex-M3 Memory Management Interrupt */
    BusFault_IRQn               = -11,    /* 5 Cortex-M3 Bus Fault Interrupt         */
    UsageFault_IRQn             = -10,    /* 6 Cortex-M3 Usage Fault Interrupt       */
    SVCall_IRQn                 = -5,     /* 11 Cortex-M3 SV Call Interrupt          */
    DebugMonitor_IRQn           = -4,     /* 12 Cortex-M3 Debug Monitor Interrupt    */
    PendSV_IRQn                 = -2,     /* 14 Cortex-M3 Pend SV Interrupt          */
    SysTick_IRQn                = -1,     /* 15 Cortex-M3 System Tick Interrupt      */

    // XXX irq list here

    NUM_IRQn,
} IRQn_Type;

#define __NVIC_PRIO_BITS 3

