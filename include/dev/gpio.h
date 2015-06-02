/*
 * Copyright (c) 2008, Google Inc.
 * Copyright (c) 2012, Travis Geiselbrecht
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef __DEV_GPIO_H
#define __DEV_GPIO_H

enum gpio_flags {
    GPIO_INPUT      = (1 << 0),
    GPIO_OUTPUT     = (1 << 1),
    GPIO_LEVEL      = (1 << 2),
    GPIO_EDGE       = (1 << 3),
    GPIO_RISING     = (1 << 4),
    GPIO_FALLING    = (1 << 5),
    GPIO_HIGH       = (1 << 6),
    GPIO_LOW        = (1 << 7),
    GPIO_PULLUP     = (1 << 8),
    GPIO_PULLDOWN   = (1 << 9),
};

/* top 16 bits of the gpio flags are platform specific */
#define GPIO_PLATFORM_MASK 0xffff0000

int gpio_config(unsigned nr, unsigned flags);
void gpio_set(unsigned nr, unsigned on);
int gpio_get(unsigned nr);

#endif
