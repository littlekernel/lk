/*
 * Copyright (c) 2019 Travis Geiselbrecht
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

#include <lk/compiler.h>

__BEGIN_CDECLS

void x86_feature_init(void);

enum x86_cpu_level {
    X86_CPU_LEVEL_386 = 3,
    X86_CPU_LEVEL_486 = 4,
    X86_CPU_LEVEL_PENTIUM = 5,
    X86_CPU_LEVEL_PENTIUM_PRO = 6,
    // everything after this is PPRO+ for now
};
extern enum x86_cpu_level __x86_cpu_level;

enum x86_cpu_vendor {
    X86_CPU_VENDOR_UNKNOWN,
    X86_CPU_VENDOR_INTEL,
    X86_CPU_VENDOR_AMD,
    X86_CPU_VENDOR_UMC,
    X86_CPU_VENDOR_CYRIX,
    X86_CPU_VENDOR_NEXGEN,
    X86_CPU_VENDOR_CENTAUR,
    X86_CPU_VENDOR_RISE,
    X86_CPU_VENDOR_SIS,
    X86_CPU_VENDOR_TRANSMETA,
    X86_CPU_VENDOR_NSC,
};
extern enum x86_cpu_vendor __x86_cpu_vendor;

static inline enum x86_cpu_level x86_get_cpu_level(void) {

    return __x86_cpu_level;
}

static inline enum x86_cpu_vendor x86_get_cpu_vendor(void) {
    return __x86_cpu_vendor;
}


__END_CDECLS

