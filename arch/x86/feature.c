/*
 * Copyright (c) 2019 Travis Geiselbrecht
 * Copyright 2016 The Fuchsia Authors
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
#include <assert.h>
#include <string.h>

#define LOCAL_TRACE 0

enum x86_cpu_vendor __x86_cpu_vendor = X86_CPU_VENDOR_INTEL;
enum x86_cpu_level __x86_cpu_level = X86_CPU_LEVEL_386; // start off assuming 386
struct x86_model_info __x86_model;

bool has_cpuid = false;

/* a saved cache of three banks of cpuids loaded a boot */
struct x86_cpuid_leaf saved_cpuids[__X86_MAX_SUPPORTED_CPUID + 1];
struct x86_cpuid_leaf saved_cpuids_hyp[__X86_MAX_SUPPORTED_CPUID_HYP - X86_CPUID_HYP_BASE + 1];
struct x86_cpuid_leaf saved_cpuids_ext[__X86_MAX_SUPPORTED_CPUID_EXT - X86_CPUID_EXT_BASE + 1];
uint32_t max_cpuid_leaf = 0;
uint32_t max_cpuid_leaf_hyp = 0;
uint32_t max_cpuid_leaf_ext = 0;

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
    if (X86_LEGACY) {
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
    } else {
        // at least a pentium and has cpuid
        __x86_cpu_level = X86_CPU_LEVEL_PENTIUM;
        has_cpuid = true;
    }

    if (has_cpuid) {
        uint32_t a, b, c, d;

        // read the max basic cpuid leaf
        cpuid(X86_CPUID_BASE, &a, &b, &c, &d);
        max_cpuid_leaf = MIN(a, __X86_MAX_SUPPORTED_CPUID);;

        LTRACEF("cpuid leaf 0: %#x %#x %#x %#x\n", a, b, c, d);

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

        LTRACEF("vendor string '%s' from cpuid\n", vs.str);

        // read max extended cpuid leaf
        cpuid(X86_CPUID_EXT_BASE, &a, &b, &c, &d);
        if (a >= X86_CPUID_EXT_BASE) {
            max_cpuid_leaf_ext = MIN(a, __X86_MAX_SUPPORTED_CPUID_EXT);
        }

        // read max hypervisor leaf
        cpuid(X86_CPUID_HYP_BASE, &a, &b, &c, &d);

        // Check that it's an understood hypervisor leaf
        if ((b == 0x4b4d564b && c == 0x564b4d56 && d == 0x4d) || /* KVMKVMKVM */
            (b == 0x54474354 && c == 0x43544743 && d == 0x47435447)) {  /* TCGTCGTCGTCG */
            max_cpuid_leaf_hyp = MIN(a, __X86_MAX_SUPPORTED_CPUID_HYP);
        } else {
            max_cpuid_leaf_hyp = 0;
        }
    } else {
        __x86_cpu_vendor = X86_CPU_VENDOR_INTEL; // intrinsically Intel without cpuid
    }

    // detect and populate the x86_model structure
    if (has_cpuid && max_cpuid_leaf >= 1) {
        uint32_t a, b, c, d;
        cpuid(X86_CPUID_MODEL_FEATURES, &a, &b, &c, &d);

        LTRACEF("cpuid leaf 1: %#x %#x %#x %#x\n", a, b, c, d);

        __x86_model.processor_type = BITS_SHIFT(a, 13, 12);
        __x86_model.family = BITS_SHIFT(a, 11, 8);
        __x86_model.model = BITS_SHIFT(a, 7, 4);
        __x86_model.stepping = BITS_SHIFT(a, 3, 0);
        __x86_model.display_family = __x86_model.family;
        __x86_model.display_model = __x86_model.model;

        uint32_t ext_family = BITS_SHIFT(a, 27, 20);
        uint32_t ext_model = BITS_SHIFT(a, 19, 16);

        switch (__x86_model.family) {
           case 4:
                __x86_cpu_level = X86_CPU_LEVEL_486;
                break;
           case 5:
                __x86_cpu_level = X86_CPU_LEVEL_PENTIUM;
                break;
           case 6:
                __x86_cpu_level = X86_CPU_LEVEL_PENTIUM_PRO;
                if (x86_get_cpu_vendor() == X86_CPU_VENDOR_INTEL) {
                    __x86_model.display_model |= ext_model << 4; // extended model field extends the regular model
                }
                break;
           case 0xf:
                __x86_cpu_level = X86_CPU_LEVEL_PENTIUM_PRO;
                __x86_model.display_family += ext_family; // family 0xf stuff is extended by bits 27:20
                __x86_model.display_model |= ext_model << 4; // extended model field extends the regular model
                break;
           default:
                // unhandled decode, assume ppro+ level
                __x86_cpu_level = X86_CPU_LEVEL_PENTIUM_PRO;
                break;
        }
    }
}

/* early detection of cpu features on cpu 0, before the kernel is scheduling */
void x86_feature_early_init(void) {
    x86_cpu_detect();

    // cache a copy of the cpuid bits
    if (has_cpuid) {
        for (uint32_t i = 0; i <= max_cpuid_leaf; i++) {
            cpuid_c(i, 0, &saved_cpuids[i].a, &saved_cpuids[i].b, &saved_cpuids[i].c, &saved_cpuids[i].d);
        }

        if (max_cpuid_leaf_ext > 0) {
            for (uint32_t i = X86_CPUID_EXT_BASE; i <= max_cpuid_leaf_ext; i++) {
                uint32_t index = i - X86_CPUID_EXT_BASE;
                cpuid_c(i, 0, &saved_cpuids_ext[index].a, &saved_cpuids_ext[index].b, &saved_cpuids_ext[index].c,
                        &saved_cpuids_ext[index].d);
            }
        }

        if (max_cpuid_leaf_hyp > 0) {
            for (uint32_t i = X86_CPUID_HYP_BASE; i <= max_cpuid_leaf_hyp; i++) {
                uint32_t index = i - X86_CPUID_HYP_BASE;
                cpuid_c(i, 0, &saved_cpuids_hyp[index].a, &saved_cpuids_hyp[index].b, &saved_cpuids_hyp[index].c,
                        &saved_cpuids_hyp[index].d);
            }
        }
    }
}

static void x86_feature_dump_cpuid(void) {
    for (uint32_t i = X86_CPUID_BASE; i <= max_cpuid_leaf; i++) {
        printf("X86: cpuid leaf %#x: %08x %08x %08x %08x\n", i,
               saved_cpuids[i - X86_CPUID_BASE].a, saved_cpuids[i - X86_CPUID_BASE].b, saved_cpuids[i - X86_CPUID_BASE].c, saved_cpuids[i - X86_CPUID_BASE].d);
    }
    for (uint32_t i = X86_CPUID_HYP_BASE; i <= max_cpuid_leaf_hyp; i++) {
        uint32_t index = i - X86_CPUID_HYP_BASE;
        printf("X86: cpuid leaf %#x: %08x %08x %08x %08x\n", i,
               saved_cpuids_hyp[index].a, saved_cpuids_hyp[index].b, saved_cpuids_hyp[index].c, saved_cpuids_hyp[index].d);
    }
    for (uint32_t i = X86_CPUID_EXT_BASE; i <= max_cpuid_leaf_ext; i++) {
        uint32_t index = i - X86_CPUID_EXT_BASE;
        printf("X86: cpuid leaf %#x: %08x %08x %08x %08x\n", i,
               saved_cpuids[index].a, saved_cpuids[index].b, saved_cpuids[index].c, saved_cpuids[index].d);
    }
}

/* later feature init hook, called after the kernel is able to schedule */
void x86_feature_init(void) {
    dprintf(SPEW, "X86: detected cpu level %d has_cpuid %d\n", x86_get_cpu_level(), has_cpuid);
    if (has_cpuid) {
        dprintf(SPEW, "X86: max cpuid leaf %#x ext %#x hyp %#x\n",
                max_cpuid_leaf, max_cpuid_leaf_ext, max_cpuid_leaf_hyp);
    }

    if (has_cpuid) {
        // read the max basic cpuid leaf
        uint32_t a, b, c, d;
        cpuid(X86_CPUID_BASE, &a, &b, &c, &d);

        // read the vendor string
        union {
            uint32_t reg[3];
            char str[13];
        } vs;
        vs.reg[0] = b;
        vs.reg[1] = d;
        vs.reg[2] = c;
        vs.str[12] = 0;

        dprintf(SPEW, "X86: vendor string '%s'\n", vs.str);
    }

    const struct x86_model_info* model = x86_get_model();
    printf("X86: processor model info type %#x family %#x model %#x stepping %#x\n",
           model->processor_type, model->family, model->model, model->stepping);
    printf("\tdisplay_family %#x display_model %#x\n", model->display_family, model->display_model);

    if (has_cpuid && LK_DEBUGLEVEL > 1) {
        x86_feature_dump_cpuid();
    }
}

bool x86_get_cpuid_subleaf(enum x86_cpuid_leaf_num num, uint32_t subleaf, struct x86_cpuid_leaf* leaf) {
  // make sure the leaf number is within the detected range of the three blocks we know about
  if (num < X86_CPUID_HYP_BASE) {
    if (num > max_cpuid_leaf) {
      return false;
    }
  } else if (num < X86_CPUID_EXT_BASE) {
    if (num > max_cpuid_leaf_hyp) {
      return false;
    }
  } else if (num > max_cpuid_leaf_ext) {
    return false;
  }

  cpuid_c((uint32_t)num, subleaf, &leaf->a, &leaf->b, &leaf->c, &leaf->d);
  return true;
}


