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
#include <arch/x86/feature.h>

#include <lk/bits.h>
#include <lk/debug.h>
#include <lk/trace.h>
#include <arch/x86.h>
#include <string.h>

#define LOCAL_TRACE 0

enum x86_cpu_vendor __x86_cpu_vendor = X86_CPU_VENDOR_INTEL;
enum x86_cpu_level __x86_cpu_level = X86_CPU_LEVEL_386; // start off assuming 386
uint32_t max_cpuid_leaf = 0;
uint32_t max_cpuid_leaf_extended = 0;

static enum x86_cpu_vendor match_cpu_vendor_string(const char *str) {
    // from table at https://www.sandpile.org/x86/cpuid.htm#level_0000_0000h
    if (!strcmp(str, "GenuineIntel")) {
        return X86_CPU_VENDOR_INTEL;
    }
    if (!strcmp(str, "UMC UMC UMC ")) {
        return X86_CPU_VENDOR_UMC;
    }
    if (!strcmp(str, "AuthenticAMD")) {
        return X86_CPU_VENDOR_AMD;
    }
    if (!strcmp(str, "CyrixInstead")) {
        return X86_CPU_VENDOR_CYRIX;
    }
    if (!strcmp(str, "NexGenDriven")) {
        return X86_CPU_VENDOR_NEXGEN;
    }
    if (!strcmp(str, "CentaurHauls")) {
        return X86_CPU_VENDOR_CENTAUR;
    }
    if (!strcmp(str, "RiseRiseRise")) {
        return X86_CPU_VENDOR_RISE;
    }
    if (!strcmp(str, "SiS SiS SiS ")) {
        return X86_CPU_VENDOR_SIS;
    }
    if (!strcmp(str, "GenuineTMx86")) {
        return X86_CPU_VENDOR_TRANSMETA;
    }
    if (!strcmp(str, "Geode by NSC")) {
        return X86_CPU_VENDOR_NSC;
    }
    return X86_CPU_VENDOR_UNKNOWN;
}

static void x86_cpu_detect(void) {
    bool has_cpuid = false;

#if X86_LEGACY
    // inspired by http://www.rcollins.org/ddj/Sep96/Sep96.html
    // try to detect a 486
    // set the EFLAGS.AC bit, see if it sets
    uint32_t flags = x86_save_flags();
    x86_restore_flags(flags | X86_FLAGS_AC);
    if (x86_save_flags() & X86_FLAGS_AC) {
        __x86_cpu_level = X86_CPU_LEVEL_486;

        // test EFLAGS.ID flag
        x86_restore_flags(flags | X86_FLAGS_ID);
        if (x86_save_flags() & X86_FLAGS_ID) {
            has_cpuid = true;
        }
    }
#else
    // at least a pentium and has cpuid
    __x86_cpu_level = X86_CPU_LEVEL_PENTIUM;
    has_cpuid = true;
    uint32_t a, b, c, d;

    // read the max basic cpuid leaf
    cpuid(0, &a, &b, &c, &d);
    max_cpuid_leaf = a;

    // read the vendor string
    union {
        uint32_t reg[3];
        char str[13];
    } vs;
    vs.reg[0] = b;
    vs.reg[1] = d;
    vs.reg[2] = c;
    vs.str[12] = 0;
    __x86_cpu_vendor = match_cpu_vendor_string(vs.str);

    // read max extended cpuid leaf
    cpuid(0x80000000, &a, &b, &c, &d);
    if (a >= 0x80000000) {
        max_cpuid_leaf_extended = a;
    }

    dprintf(SPEW, "x86: vendor string '%s'\n", vs.str);

    // do a quick cpu level detection using cpuid
    if (max_cpuid_leaf >= 1) {
        cpuid(1, &a, &b, &c, &d);

        LTRACEF("cpuid leaf 1: %#x %#x %#x %#x\n", a, b, c, d);

        uint32_t ext_family = BITS_SHIFT(a, 27, 20);
        uint32_t ext_model = BITS_SHIFT(a, 19, 16);
        uint32_t family = BITS_SHIFT(a, 11, 8);
        uint32_t model = BITS_SHIFT(a, 7, 4);
        LTRACEF("raw family %#x model %#x ext_family %#x ext_model %#x\n", family, model, ext_family, ext_model);

        switch (family) {
           case 4:
                __x86_cpu_level = X86_CPU_LEVEL_486;
                break;
           case 5:
                __x86_cpu_level = X86_CPU_LEVEL_PENTIUM;
                break;
           case 6:
                __x86_cpu_level = X86_CPU_LEVEL_PENTIUM_PRO;
                if (x86_get_cpu_vendor() == X86_CPU_VENDOR_INTEL) {
                    model |= ext_model << 4; // extended model field extends the regular model
                }
                break;
           case 0xf:
                __x86_cpu_level = X86_CPU_LEVEL_PENTIUM_PRO;
                family += ext_family; // family 0xf stuff is extended by bits 27:20
                model |= ext_model << 4; // extended model field extends the regular model
                break;
           default:
                // unhandled decode, assume ppro+ level
                __x86_cpu_level = X86_CPU_LEVEL_PENTIUM_PRO;
                break;
        }
        dprintf(SPEW, "x86: family %#x model %#x\n", family, model);

        // TODO: save this information for future use
    }
#endif

    dprintf(SPEW, "x86: detected cpu level %d has_cpuid %d\n", x86_get_cpu_level(), has_cpuid);
    if (has_cpuid) {
        dprintf(SPEW, "x86: max cpuid leaf %#x ext %#x\n", max_cpuid_leaf, max_cpuid_leaf_extended);
    }
}

void x86_feature_init(void) {
    x86_cpu_detect();
}

