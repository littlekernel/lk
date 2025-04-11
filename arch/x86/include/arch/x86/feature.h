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
#pragma once

#include <arch/x86.h>
#include <lk/compiler.h>
#include <stdbool.h>
#include <assert.h>

__BEGIN_CDECLS

void x86_feature_early_init(void);
void x86_feature_init(void);

enum x86_cpu_level {
    X86_CPU_LEVEL_386 = 3,
    X86_CPU_LEVEL_486 = 4,
    X86_CPU_LEVEL_PENTIUM = 5,
    X86_CPU_LEVEL_PENTIUM_PRO = 6,
    // everything after this is PPRO+ for now
};
extern enum x86_cpu_level __x86_cpu_level;

static inline enum x86_cpu_level x86_get_cpu_level(void) {
    return __x86_cpu_level;
}

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

static inline enum x86_cpu_vendor x86_get_cpu_vendor(void) {
    return __x86_cpu_vendor;
}

struct x86_model_info {
  uint8_t processor_type;
  uint8_t family;
  uint8_t model;
  uint8_t stepping;

  uint32_t display_family;
  uint32_t display_model;
};
extern struct x86_model_info __x86_model;

static inline const struct x86_model_info* x86_get_model(void) {
    return &__x86_model;
}

/* cpuid leaves */
enum x86_cpuid_leaf_num {
  X86_CPUID_BASE = 0,
  X86_CPUID_MODEL_FEATURES = 0x1,
  X86_CPUID_CACHE_V1 = 0x2,
  X86_CPUID_CACHE_V2 = 0x4,
  X86_CPUID_MON = 0x5,
  X86_CPUID_THERMAL_AND_POWER = 0x6,
  X86_CPUID_EXTENDED_FEATURE_FLAGS = 0x7,
  X86_CPUID_PERFORMANCE_MONITORING = 0xa,
  X86_CPUID_TOPOLOGY = 0xb,
  X86_CPUID_XSAVE = 0xd,
  X86_CPUID_PT = 0x14,
  X86_CPUID_TSC = 0x15,
  __X86_MAX_SUPPORTED_CPUID = X86_CPUID_TSC,

  X86_CPUID_HYP_BASE = 0x40000000,
  X86_CPUID_HYP_VENDOR = 0x40000000,
  X86_CPUID_KVM_FEATURES = 0x40000001,
  __X86_MAX_SUPPORTED_CPUID_HYP = X86_CPUID_KVM_FEATURES,

  X86_CPUID_EXT_BASE = 0x80000000,
  X86_CPUID_BRAND = 0x80000002,
  X86_CPUID_ADDR_WIDTH = 0x80000008,
  X86_CPUID_AMD_TOPOLOGY = 0x8000001e,
  __X86_MAX_SUPPORTED_CPUID_EXT = X86_CPUID_AMD_TOPOLOGY,
};

struct x86_cpuid_bit {
  enum x86_cpuid_leaf_num leaf_num;
  uint8_t word;
  uint8_t bit;
};

#define X86_CPUID_BIT(leaf, word, bit) \
  (struct x86_cpuid_bit) { (enum x86_cpuid_leaf_num)(leaf), (word), (bit) }

struct x86_cpuid_leaf {
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t d;
};

extern struct x86_cpuid_leaf saved_cpuids[__X86_MAX_SUPPORTED_CPUID + 1];
extern struct x86_cpuid_leaf saved_cpuids_hyp[__X86_MAX_SUPPORTED_CPUID_HYP - X86_CPUID_HYP_BASE + 1];
extern struct x86_cpuid_leaf saved_cpuids_ext[__X86_MAX_SUPPORTED_CPUID_EXT - X86_CPUID_EXT_BASE + 1];
extern uint32_t max_cpuid_leaf;
extern uint32_t max_cpuid_leaf_hyp;
extern uint32_t max_cpuid_leaf_ext;

/* Retrieve the specified subleaf.  This function is not cached.
 * Returns false if leaf num is invalid */
bool x86_get_cpuid_subleaf(enum x86_cpuid_leaf_num, uint32_t subleaf, struct x86_cpuid_leaf *);

static inline const struct x86_cpuid_leaf* x86_get_cpuid_leaf(enum x86_cpuid_leaf_num leaf) {
  if (leaf < X86_CPUID_HYP_BASE) {
    if (unlikely(leaf > max_cpuid_leaf))
      return NULL;

    return &saved_cpuids[leaf];
  } else if (leaf < X86_CPUID_EXT_BASE) {
    if (unlikely(leaf > max_cpuid_leaf_hyp))
      return NULL;

    return &saved_cpuids_hyp[(uint32_t)leaf - (uint32_t)X86_CPUID_HYP_BASE];
  } else {
    if (unlikely(leaf > max_cpuid_leaf_ext))
      return NULL;

    return &saved_cpuids_ext[(uint32_t)leaf - (uint32_t)X86_CPUID_EXT_BASE];
  }
}

static inline bool x86_feature_test(struct x86_cpuid_bit bit) {
  DEBUG_ASSERT(bit.word <= 3 && bit.bit <= 31);

  if (bit.word > 3 || bit.bit > 31)
    return false;

  const struct x86_cpuid_leaf* leaf = x86_get_cpuid_leaf(bit.leaf_num);
  if (!leaf)
    return false;

  switch (bit.word) {
    case 0:
      return !!((1u << bit.bit) & leaf->a);
    case 1:
      return !!((1u << bit.bit) & leaf->b);
    case 2:
      return !!((1u << bit.bit) & leaf->c);
    case 3:
      return !!((1u << bit.bit) & leaf->d);
    default:
      return false;
  }
}

/* feature bits for x86_feature_test */
/* add feature bits to test here */
/* format: X86_CPUID_BIT(cpuid leaf, register (eax-edx:0-3), bit) */
#define X86_FEATURE_SSE3                X86_CPUID_BIT(0x1, 2, 0)
#define X86_FEATURE_PCLMULQDQ           X86_CPUID_BIT(0x1, 2, 1)
#define X86_FEATURE_DTES64              X86_CPUID_BIT(0x1, 2, 2)
#define X86_FEATURE_MON                 X86_CPUID_BIT(0x1, 2, 3)
#define X86_FEATURE_DSCPL               X86_CPUID_BIT(0x1, 2, 4)
#define X86_FEATURE_VMX                 X86_CPUID_BIT(0x1, 2, 5)
#define X86_FEATURE_SMX                 X86_CPUID_BIT(0x1, 2, 6)
#define X86_FEATURE_EIST                X86_CPUID_BIT(0x1, 2, 7)
#define X86_FEATURE_TM2                 X86_CPUID_BIT(0x1, 2, 8)
#define X86_FEATURE_SSSE3               X86_CPUID_BIT(0x1, 2, 9)
#define X86_FEATURE_CNXT_ID             X86_CPUID_BIT(0x1, 2, 10)
#define X86_FEATURE_SDBG                X86_CPUID_BIT(0x1, 2, 11)
#define X86_FEATURE_FMA                 X86_CPUID_BIT(0x1, 2, 12)
#define X86_FEATURE_CMPXCHG16B          X86_CPUID_BIT(0x1, 2, 13)
#define X86_FEATURE_XTPR                X86_CPUID_BIT(0x1, 2, 14)
#define X86_FEATURE_PDCM                X86_CPUID_BIT(0x1, 2, 15)
#define X86_FEATURE_PCID                X86_CPUID_BIT(0x1, 2, 17)
#define X86_FEATURE_DCA                 X86_CPUID_BIT(0x1, 2, 18)
#define X86_FEATURE_SSE4_1              X86_CPUID_BIT(0x1, 2, 19)
#define X86_FEATURE_SSE4_2              X86_CPUID_BIT(0x1, 2, 20)
#define X86_FEATURE_X2APIC              X86_CPUID_BIT(0x1, 2, 21)
#define X86_FEATURE_MOVBE               X86_CPUID_BIT(0x1, 2, 22)
#define X86_FEATURE_POPCNT              X86_CPUID_BIT(0x1, 2, 23)
#define X86_FEATURE_TSC_DEADLINE        X86_CPUID_BIT(0x1, 2, 24)
#define X86_FEATURE_AESNI               X86_CPUID_BIT(0x1, 2, 25)
#define X86_FEATURE_XSAVE               X86_CPUID_BIT(0x1, 2, 26)
#define X86_FEATURE_OSXSAVE             X86_CPUID_BIT(0x1, 2, 27)
#define X86_FEATURE_AVX                 X86_CPUID_BIT(0x1, 2, 28)
#define X86_FEATURE_F16C                X86_CPUID_BIT(0x1, 2, 29)
#define X86_FEATURE_RDRAND              X86_CPUID_BIT(0x1, 2, 30)
#define X86_FEATURE_HYPERVISOR          X86_CPUID_BIT(0x1, 2, 31)
#define X86_FEATURE_FPU                 X86_CPUID_BIT(0x1, 3, 0)
#define X86_FEATURE_VM86                X86_CPUID_BIT(0x1, 3, 1)
#define X86_FEATURE_DE                  X86_CPUID_BIT(0x1, 3, 2)
#define X86_FEATURE_PSE                 X86_CPUID_BIT(0x1, 3, 3)
#define X86_FEATURE_TSC                 X86_CPUID_BIT(0x1, 3, 4)
#define X86_FEATURE_MSR                 X86_CPUID_BIT(0x1, 3, 5)
#define X86_FEATURE_PAE                 X86_CPUID_BIT(0x1, 3, 6)
#define X86_FEATURE_MCE                 X86_CPUID_BIT(0x1, 3, 7)
#define X86_FEATURE_CX8                 X86_CPUID_BIT(0x1, 3, 8)
#define X86_FEATURE_APIC                X86_CPUID_BIT(0x1, 3, 9)
#define X86_FEATURE_SEP                 X86_CPUID_BIT(0x1, 3, 11)
#define X86_FEATURE_MTRR                X86_CPUID_BIT(0x1, 3, 12)
#define X86_FEATURE_PGE                 X86_CPUID_BIT(0x1, 3, 13)
#define X86_FEATURE_MCA                 X86_CPUID_BIT(0x1, 3, 14)
#define X86_FEATURE_CMOV                X86_CPUID_BIT(0x1, 3, 15)
#define X86_FEATURE_PAT                 X86_CPUID_BIT(0x1, 3, 16)
#define X86_FEATURE_PSE36               X86_CPUID_BIT(0x1, 3, 17)
#define X86_FEATURE_PSN                 X86_CPUID_BIT(0x1, 3, 18)
#define X86_FEATURE_CLFLUSH             X86_CPUID_BIT(0x1, 3, 19)
#define X86_FEATURE_DS                  X86_CPUID_BIT(0x1, 3, 21)
#define X86_FEATURE_ACPI                X86_CPUID_BIT(0x1, 3, 22)
#define X86_FEATURE_MMX                 X86_CPUID_BIT(0x1, 3, 23)
#define X86_FEATURE_FXSR                X86_CPUID_BIT(0x1, 3, 24)
#define X86_FEATURE_SSE                 X86_CPUID_BIT(0x1, 3, 25)
#define X86_FEATURE_SSE2                X86_CPUID_BIT(0x1, 3, 26)
#define X86_FEATURE_SS                  X86_CPUID_BIT(0x1, 3, 27)
#define X86_FEATURE_HTT                 X86_CPUID_BIT(0x1, 3, 28)
#define X86_FEATURE_TM                  X86_CPUID_BIT(0x1, 3, 29)
#define X86_FEATURE_PBE                 X86_CPUID_BIT(0x1, 3, 31)

#define X86_FEATURE_DTS                 X86_CPUID_BIT(0x6, 0, 0)
#define X86_FEATURE_TURBO               X86_CPUID_BIT(0x6, 0, 1)
#define X86_FEATURE_ARAT                X86_CPUID_BIT(0x6, 0, 2)
#define X86_FEATURE_PLN                 X86_CPUID_BIT(0x6, 0, 4)
#define X86_FEATURE_ECMD                X86_CPUID_BIT(0x6, 0, 5)
#define X86_FEATURE_PTM                 X86_CPUID_BIT(0x6, 0, 6)
#define X86_FEATURE_HWP                 X86_CPUID_BIT(0x6, 0, 7)
#define X86_FEATURE_HWP_NOT             X86_CPUID_BIT(0x6, 0, 8)
#define X86_FEATURE_HWP_ACT             X86_CPUID_BIT(0x6, 0, 9)
#define X86_FEATURE_HWP_EPP             X86_CPUID_BIT(0x6, 0, 10)
#define X86_FEATURE_HWP_PKG             X86_CPUID_BIT(0x6, 0, 11)
#define X86_FEATURE_HDC                 X86_CPUID_BIT(0x6, 0, 13)
#define X86_FEATURE_TURBO_MAX           X86_CPUID_BIT(0x6, 0, 14)
#define X86_FEATURE_HWP_CAP             X86_CPUID_BIT(0x6, 0, 15)
#define X86_FEATURE_HWP_PECI            X86_CPUID_BIT(0x6, 0, 16)
#define X86_FEATURE_HWP_FLEX            X86_CPUID_BIT(0x6, 0, 17)
#define X86_FEATURE_HWP_FAST            X86_CPUID_BIT(0x6, 0, 18)
#define X86_FEATURE_HW_FEEDBACK         X86_CPUID_BIT(0x6, 0, 19)
#define X86_FEATURE_THREAD_DIR          X86_CPUID_BIT(0x6, 0, 23)
#define X86_FEATURE_MPERF               X86_CPUID_BIT(0x6, 2, 0)
#define X86_FEATURE_PERF_BIAS           X86_CPUID_BIT(0x6, 2, 3)

#define X86_FEATURE_FSGSBASE            X86_CPUID_BIT(0x7, 1, 0)
#define X86_FEATURE_TSC_ADJUST          X86_CPUID_BIT(0x7, 1, 1)
#define X86_FEATURE_SGX                 X86_CPUID_BIT(0x7, 1, 2)
#define X86_FEATURE_BMI1                X86_CPUID_BIT(0x7, 1, 3)
#define X86_FEATURE_HLE                 X86_CPUID_BIT(0x7, 1, 4)
#define X86_FEATURE_AVX2                X86_CPUID_BIT(0x7, 1, 5)
#define X86_FEATURE_FPDP                X86_CPUID_BIT(0x7, 1, 6)
#define X86_FEATURE_SMEP                X86_CPUID_BIT(0x7, 1, 7)
#define X86_FEATURE_BMI2                X86_CPUID_BIT(0x7, 1, 8)
#define X86_FEATURE_ERMS                X86_CPUID_BIT(0x7, 1, 9)
#define X86_FEATURE_INVPCID             X86_CPUID_BIT(0x7, 1, 10)
#define X86_FEATURE_RTM                 X86_CPUID_BIT(0x7, 1, 11)
#define X86_FEATURE_PQM                 X86_CPUID_BIT(0x7, 1, 12)
#define X86_FEATURE_FPCSDS              X86_CPUID_BIT(0x7, 1, 13)
#define X86_FEATURE_MPX                 X86_CPUID_BIT(0x7, 1, 14)
#define X86_FEATURE_PQE                 X86_CPUID_BIT(0x7, 1, 15)
#define X86_FEATURE_AVX512F             X86_CPUID_BIT(0x7, 1, 16)
#define X86_FEATURE_AVX512DQ            X86_CPUID_BIT(0x7, 1, 17)
#define X86_FEATURE_RDSEED              X86_CPUID_BIT(0x7, 1, 18)
#define X86_FEATURE_ADX                 X86_CPUID_BIT(0x7, 1, 19)
#define X86_FEATURE_SMAP                X86_CPUID_BIT(0x7, 1, 20)
#define X86_FEATURE_AVX512IFMA          X86_CPUID_BIT(0x7, 1, 21)
#define X86_FEATURE_CLFLUSHOPT          X86_CPUID_BIT(0x7, 1, 23)
#define X86_FEATURE_CLWB                X86_CPUID_BIT(0x7, 1, 24)
#define X86_FEATURE_PT                  X86_CPUID_BIT(0x7, 1, 25)
#define X86_FEATURE_AVX512PF            X86_CPUID_BIT(0x7, 1, 26)
#define X86_FEATURE_AVX512ER            X86_CPUID_BIT(0x7, 1, 27)
#define X86_FEATURE_AVX512CD            X86_CPUID_BIT(0x7, 1, 28)
#define X86_FEATURE_SHA                 X86_CPUID_BIT(0x7, 1, 29)
#define X86_FEATURE_AVX512BW            X86_CPUID_BIT(0x7, 1, 30)
#define X86_FEATURE_AVX512VL            X86_CPUID_BIT(0x7, 1, 31)
#define X86_FEATURE_PREFETCHWT1         X86_CPUID_BIT(0x7, 2, 0)
#define X86_FEATURE_AVX512VBMI          X86_CPUID_BIT(0x7, 2, 1)
#define X86_FEATURE_UMIP                X86_CPUID_BIT(0x7, 2, 2)
#define X86_FEATURE_PKU                 X86_CPUID_BIT(0x7, 2, 3)
#define X86_FEATURE_OSPKE               X86_CPUID_BIT(0x7, 2, 4)
#define X86_FEATURE_WAITPKG             X86_CPUID_BIT(0x7, 2, 5)
#define X86_FEATURE_AVX512_VBMI2        X86_CPUID_BIT(0x7, 2, 6)
#define X86_FEATURE_CET_SS              X86_CPUID_BIT(0x7, 2, 7)
#define X86_FEATURE_GFNI                X86_CPUID_BIT(0x7, 2, 8)
#define X86_FEATURE_VAES                X86_CPUID_BIT(0x7, 2, 9)
#define X86_FEATURE_VPCLMULQDQ          X86_CPUID_BIT(0x7, 2, 10)
#define X86_FEATURE_AVX512_VNNI         X86_CPUID_BIT(0x7, 2, 11)
#define X86_FEATURE_AVX512_BITALG       X86_CPUID_BIT(0x7, 2, 12)
#define X86_FEATURE_TIME_EN             X86_CPUID_BIT(0x7, 2, 13)
#define X86_FEATURE_AVX512_VPOPCNTDQ    X86_CPUID_BIT(0x7, 2, 14)
#define X86_FEATURE_LA57                X86_CPUID_BIT(0x7, 2, 16)
#define X86_FEATURE_RDPID               X86_CPUID_BIT(0x7, 2, 22)
#define X86_FEATURE_KL                  X86_CPUID_BIT(0x7, 2, 23)
#define X86_FEATURE_CLDEMOTE            X86_CPUID_BIT(0x7, 2, 25)
#define X86_FEATURE_MOVDIRI             X86_CPUID_BIT(0x7, 2, 27)
#define X86_FEATURE_MOVDIR64B           X86_CPUID_BIT(0x7, 2, 28)
#define X86_FEATURE_SGX_LC              X86_CPUID_BIT(0x7, 2, 30)
#define X86_FEATURE_PKS                 X86_CPUID_BIT(0x7, 2, 31)
#define X86_FEATURE_AVX512_4VNNIW       X86_CPUID_BIT(0x7, 3, 2)
#define X86_FEATURE_AVX512_4FMAPS       X86_CPUID_BIT(0x7, 3, 3)
#define X86_FEATURE_FSRM                X86_CPUID_BIT(0x7, 3, 4)
#define X86_FEATURE_AVX512_VP2INTERSECT X86_CPUID_BIT(0x7, 3, 8)
#define X86_FEATURE_MD_CLEAR            X86_CPUID_BIT(0x7, 3, 10)
#define X86_FEATURE_SERIALIZE           X86_CPUID_BIT(0x7, 3, 14)
#define X86_FEATURE_HYBRID              X86_CPUID_BIT(0x7, 3, 15)
#define X86_FEATURE_PCONFIG             X86_CPUID_BIT(0x7, 3, 18)
#define X86_FEATURE_CET_IBT             X86_CPUID_BIT(0x7, 3, 20)
#define X86_FEATURE_IBRS_IBPB           X86_CPUID_BIT(0x7, 3, 26)
#define X86_FEATURE_STIBP               X86_CPUID_BIT(0x7, 3, 27)
#define X86_FEATURE_L1D_FLUSH           X86_CPUID_BIT(0x7, 3, 28)
#define X86_FEATURE_ARCH_CAPABILITIES   X86_CPUID_BIT(0x7, 3, 29)
#define X86_FEATURE_CORE_CAPABILITIES   X86_CPUID_BIT(0x7, 3, 30)
#define X86_FEATURE_SSBD                X86_CPUID_BIT(0x7, 3, 31)

#define X86_FEATURE_KVM_CLOCKSOURCE     X86_CPUID_BIT(0x40000001, 0, 0)
#define X86_FEATURE_KVM_NOP_IO_DELAY    X86_CPUID_BIT(0x40000001, 0, 1)
#define X86_FEATURE_KVM_MMU_OP          X86_CPUID_BIT(0x40000001, 0, 2)
#define X86_FEATURE_KVM_CLOCKSOURCE2    X86_CPUID_BIT(0x40000001, 0, 3)
#define X86_FEATURE_KVM_ASYNC_PF        X86_CPUID_BIT(0x40000001, 0, 4)
#define X86_FEATURE_KVM_STEAL_TIME      X86_CPUID_BIT(0x40000001, 0, 5)
#define X86_FEATURE_KVM_PV_EOI          X86_CPUID_BIT(0x40000001, 0, 6)
#define X86_FEATURE_KVM_PV_UNHALT       X86_CPUID_BIT(0x40000001, 0, 7)
#define X86_FEATURE_KVM_PV_TLB_FLUSH    X86_CPUID_BIT(0x40000001, 0, 9)
#define X86_FEATURE_KVM_ASYNC_PF_VMEXIT X86_CPUID_BIT(0x40000001, 0, 10)
#define X86_FEATURE_KVM_PV_IPI          X86_CPUID_BIT(0x40000001, 0, 11)
#define X86_FEATURE_KVM_POLL_CONTROL    X86_CPUID_BIT(0x40000001, 0, 12)
#define X86_FEATURE_KVM_PV_SCHED_YIELD  X86_CPUID_BIT(0x40000001, 0, 13)
#define X86_FEATURE_KVM_ASYNC_PF_INT    X86_CPUID_BIT(0x40000001, 0, 14)
#define X86_FEATURE_KVM_MSI_EXT_DEST_ID X86_CPUID_BIT(0x40000001, 0, 15)
#define X86_FEATURE_KVM_HC_MAP_GPA_RANGE X86_CPUID_BIT(0x40000001, 0, 16)
#define X86_FEATURE_KVM_MIGRATION_CONTROL X86_CPUID_BIT(0x40000001, 0, 17)
#define X86_FEATURE_KVM_CLOCKSOURCE_STABLE X86_CPUID_BIT(0x40000001, 0, 24)


#define X86_FEATURE_AHF64               X86_CPUID_BIT(0x80000001, 2, 0)
#define X86_FEATURE_LZCNT               X86_CPUID_BIT(0x80000001, 2, 5)
#define X86_FEATURE_PREFETCHW           X86_CPUID_BIT(0x80000001, 2, 8)
#define X86_FEATURE_AMD_TOPO            X86_CPUID_BIT(0x80000001, 2, 22)
#define X86_FEATURE_SSE4A               X86_CPUID_BIT(0x80000001, 3, 6)
#define X86_FEATURE_SYSCALL             X86_CPUID_BIT(0x80000001, 3, 11)
#define X86_FEATURE_NX                  X86_CPUID_BIT(0x80000001, 3, 20)
#define X86_FEATURE_PG1G                X86_CPUID_BIT(0x80000001, 3, 26)
#define X86_FEATURE_RDTSCP              X86_CPUID_BIT(0x80000001, 3, 27)
#define X86_FEATURE_LM                  X86_CPUID_BIT(0x80000001, 3, 29)
#define X86_FEATURE_INVAR_TSC           X86_CPUID_BIT(0x80000007, 3, 8)

// accessor to read some fields out of a register
static inline uint32_t x86_get_vaddr_width(void) {
    const struct x86_cpuid_leaf *leaf;

    leaf = x86_get_cpuid_leaf(X86_CPUID_ADDR_WIDTH);
    if (!leaf) {
        return 0;
    }
    return (leaf->a >> 8) & 0xff;
}

static inline uint32_t x86_get_paddr_width(void) {
    const struct x86_cpuid_leaf *leaf;

    leaf = x86_get_cpuid_leaf(X86_CPUID_ADDR_WIDTH);
    if (!leaf) {
        return 0;
    }
    return leaf->a & 0xff;
}

__END_CDECLS
