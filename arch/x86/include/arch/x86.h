/*
 * Copyright (c) 2009 Corey Tabaka
 * Copyright (c) 2015 Intel Corporation
 * Copyright (c) 2016 Travis Geiselbrecht
 * Copyright 2016 The Fuchsia Authors
 *
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdbool.h>

__BEGIN_CDECLS

#define PFEX_P 0x01
#define PFEX_W 0x02
#define PFEX_U 0x04
#define PFEX_RSV 0x08
#define PFEX_I 0x10
#define X86_8BYTE_MASK 0xFFFFFFFF

struct x86_32_iframe {
    uint32_t di, si, bp, sp, bx, dx, cx, ax;            // pushed by common handler using pusha
    uint32_t ds, es, fs, gs;                            // pushed by common handler
    uint32_t vector;                                    // pushed by stub
    uint32_t err_code;                                  // pushed by interrupt or stub
    uint32_t ip, cs, flags;                             // pushed by interrupt
    uint32_t user_sp, user_ss;                          // pushed by interrupt if priv change occurs
};

struct x86_64_iframe {
    uint64_t di, si, bp, bx, dx, cx, ax;                // pushed by common handler
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;      // pushed by common handler
    uint64_t vector;                                    // pushed by stub
    uint64_t err_code;                                  // pushed by interrupt or stub
    uint64_t ip, cs, flags;                             // pushed by interrupt
    uint64_t user_sp, user_ss;                          // pushed by interrupt if priv change occurs
};

#if ARCH_X86_32
typedef struct x86_32_iframe x86_iframe_t;
#elif ARCH_X86_64
typedef struct x86_64_iframe x86_iframe_t;
#endif

struct x86_32_context_switch_frame {
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t eflags;
    uint32_t eip;
};

struct x86_64_context_switch_frame {
    uint64_t r15, r14, r13, r12;
    uint64_t rbp;
    uint64_t rbx;
    uint64_t rflags;
    uint64_t rip;
};

void x86_64_context_switch(vaddr_t *oldsp, vaddr_t newsp);

/*
 * x86-32 TSS structure
 */
typedef struct {
    uint16_t    backlink, __blh;
    uint32_t    esp0;
    uint16_t    ss0, __ss0h;
    uint32_t    esp1;
    uint16_t    ss1, __ss1h;
    uint32_t    esp2;
    uint16_t    ss2, __ss2h;
    uint32_t    cr3;
    uint32_t    eip;
    uint32_t    eflags;
    uint32_t    eax, ecx, edx, ebx;
    uint32_t    esp, ebp, esi, edi;
    uint16_t    es, __esh;
    uint16_t    cs, __csh;
    uint16_t    ss, __ssh;
    uint16_t    ds, __dsh;
    uint16_t    fs, __fsh;
    uint16_t    gs, __gsh;
    uint16_t    ldt, __ldth;
    uint16_t    trace, bitmap;

    uint8_t tss_bitmap[8192];
} __PACKED tss_32_t;

/*
 * x86-64 TSS structure
 */
typedef struct {
    uint32_t rsvd0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint32_t rsvd1;
    uint32_t rsvd2;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint32_t rsvd3;
    uint32_t rsvd4;
    uint16_t rsvd5;
    uint16_t iomap_base;
} __PACKED tss_64_t;

#if ARCH_X86_32
typedef tss_32_t tss_t;
#elif ARCH_X86_64
typedef tss_64_t tss_t;
#endif

/* x86 register bits */
#define X86_CR0_PE                      (1U<<0)  /* protected mode enable */
#define X86_CR0_MP                      (1U<<1)  /* monitor coprocessor */
#define X86_CR0_EM                      (1U<<2)  /* emulation */
#define X86_CR0_TS                      (1U<<3)  /* task switched */
#define X86_CR0_ET                      (1U<<4)  /* extension type */
#define X86_CR0_NE                      (1U<<5)  /* enable x87 exception */
#define X86_CR0_WP                      (1U<<16) /* supervisor write protect */
#define X86_CR0_AM                      (1U<<18) /* alignment mask */
#define X86_CR0_NW                      (1U<<29) /* not write-through */
#define X86_CR0_CD                      (1U<<30) /* cache disable */
#define X86_CR0_PG                      (1U<<31) /* enable paging */

#define X86_CR4_VME                     (1U<<0)  /* Virtual-8086 mode extensions */
#define X86_CR4_PVI                     (1U<<1)  /* Protected Mode Virtual Interrupts */
#define X86_CR4_TSD                     (1U<<2)  /* Time stamp disable*/
#define X86_CR4_DE                      (1U<<3)  /* Debugging extensions */
#define X86_CR4_PSE                     (1U<<4)  /* Page Size Extensions */
#define X86_CR4_PAE                     (1U<<5)  /* PAE paging */
#define X86_CR4_MCE                     (1U<<6)  /* Machine Check Enable */
#define X86_CR4_PGE                     (1U<<7)  /* Page Global Enable */
#define X86_CR4_PCE                     (1U<<8)  /* Performance Monitoring Counter Enable */
#define X86_CR4_OSFXSR                  (1U<<9)  /* os supports fxsave */
#define X86_CR4_OSXMMEXPT               (1U<<10) /* os supports xmm exception */
#define X86_CR4_UMIP                    (1U<<11) /* User-mode instruction prevention */
#define X86_CR4_LA57                    (1U<<12) /* 57-bit Linear Addresses */
#define X86_CR4_VMXE                    (1U<<13) /* enable vmx */
#define X86_CR4_SMXE                    (1U<<14) /* enable smx */
#define X86_CR4_FSGSBASE                (1U<<16) /* enable {rd,wr}{fs,gs}base */
#define X86_CR4_PCIDE                   (1U<<17) /* Process-context ID enable  */
#define X86_CR4_OSXSAVE                 (1U<<18) /* os supports xsave */
#define X86_CR4_KL                      (1U<<19) /* key locker enable */
#define X86_CR4_SMEP                    (1U<<20) /* SMEP protection enabling */
#define X86_CR4_SMAP                    (1U<<21) /* SMAP protection enabling */
#define X86_CR4_PKE                     (1U<<22) /* Enable protection keys for user mode pages */
#define X86_CR4_CET                     (1U<<23) /* Control flow enforcement */
#define X86_CR4_PKS                     (1U<<24) /* Enable protection keys for supervisor mode pages */

#define X86_EFER_SCE                    (1U<<0)  /* enable SYSCALL */
#define X86_EFER_LME                    (1U<<8)  /* long mode enable */
#define X86_EFER_LMA                    (1U<<10) /* long mode active */
#define X86_EFER_NXE                    (1U<<11) /* no execute enable */
#define X86_EFER_SVME                   (1U<<12) /* secure virtual machine enable */
#define X86_EFER_LMSLE                  (1U<<13) /* long mode segment limit enable */
#define X86_EFER_FFXSR                  (1U<<14) /* fast fxsave/fxrstor */
#define X86_EFER_TCE                    (1U<<15) /* translation cache extension */
#define X86_EFER_MCOMMIT                (1U<<17) /* enable mcommit instruction */
#define X86_EFER_INTWB                  (1U<<18) /* interrupt wbinvd/wbnoinvd enable */
#define X86_EFER_UAIE                   (1U<<20) /* upper address ignore enable */
#define X86_EFER_AIBRSE                 (1U<<20) /* automatic ibrs enable */

#define X86_MSR_IA32_PLATFORM_ID        0x00000017 /* platform id */
#define X86_MSR_IA32_APIC_BASE          0x0000001b /* APIC base physical address */
#define X86_MSR_IA32_TSC_ADJUST         0x0000003b /* TSC adjust */
#define X86_MSR_IA32_BIOS_SIGN_ID       0x0000008b /* BIOS update signature */
#define X86_MSR_IA32_MTRRCAP            0x000000fe /* MTRR capability */
#define X86_MSR_IA32_SYSENTER_CS        0x00000174 /* SYSENTER CS */
#define X86_MSR_IA32_SYSENTER_ESP       0x00000175 /* SYSENTER ESP */
#define X86_MSR_IA32_SYSENTER_EIP       0x00000176 /* SYSENTER EIP */
#define X86_MSR_IA32_MCG_CAP            0x00000179 /* global machine check capability */
#define X86_MSR_IA32_MCG_STATUS         0x0000017a /* global machine check status */
#define X86_MSR_IA32_MISC_ENABLE        0x000001a0 /* enable/disable misc processor features */
#define X86_MSR_IA32_TEMPERATURE_TARGET 0x000001a2 /* Temperature target */
#define X86_MSR_IA32_MTRR_PHYSBASE0     0x00000200 /* MTRR PhysBase0 */
#define X86_MSR_IA32_MTRR_PHYSMASK0     0x00000201 /* MTRR PhysMask0 */
#define X86_MSR_IA32_MTRR_PHYSMASK9     0x00000213 /* MTRR PhysMask9 */
#define X86_MSR_IA32_MTRR_DEF_TYPE      0x000002ff /* MTRR default type */
#define X86_MSR_IA32_MTRR_FIX64K_00000  0x00000250 /* MTRR FIX64K_00000 */
#define X86_MSR_IA32_MTRR_FIX16K_80000  0x00000258 /* MTRR FIX16K_80000 */
#define X86_MSR_IA32_MTRR_FIX16K_A0000  0x00000259 /* MTRR FIX16K_A0000 */
#define X86_MSR_IA32_MTRR_FIX4K_C0000   0x00000268 /* MTRR FIX4K_C0000 */
#define X86_MSR_IA32_MTRR_FIX4K_F8000   0x0000026f /* MTRR FIX4K_F8000 */
#define X86_MSR_IA32_PAT                0x00000277 /* PAT */
#define X86_MSR_IA32_TSC_DEADLINE       0x000006e0 /* TSC deadline */
#define X86_MSR_IA32_PM_ENABLE          0x00000770 /* enable/disable HWP */
#define X86_MSR_IA32_HWP_CAPABILITIES   0x00000771 /* HWP performance range enumeration */
#define X86_MSR_IA32_HWP_REQUEST        0x00000774 /* power manage control hints */
#define X86_MSR_IA32_X2APIC_BASE        0x00000800 /* X2APIC base register */
#define X86_MSR_IA32_EFER               0xc0000080 /* EFER */
#define X86_MSR_IA32_STAR               0xc0000081 /* system call address */
#define X86_MSR_IA32_LSTAR              0xc0000082 /* long mode call address */
#define X86_MSR_IA32_CSTAR              0xc0000083 /* ia32-e compat call address */
#define X86_MSR_IA32_FMASK              0xc0000084 /* system call flag mask */
#define X86_MSR_IA32_FS_BASE            0xc0000100 /* fs base address */
#define X86_MSR_IA32_GS_BASE            0xc0000101 /* gs base address */
#define X86_MSR_IA32_KERNEL_GS_BASE     0xc0000102 /* kernel gs base */
#define X86_MSR_IA32_TSC_AUX            0xc0000103 /* TSC aux */

// Non-architectural MSRs
#define X86_MSR_RAPL_POWER_UNIT         0x00000606 /* RAPL unit multipliers */
#define X86_MSR_PKG_POWER_LIMIT         0x00000610 /* Package power limits */
#define X86_MSR_PKG_POWER_LIMIT_PL1_CLAMP   (1U << 16)
#define X86_MSR_PKG_POWER_LIMIT_PL1_ENABLE  (1U << 15)
#define X86_MSR_PKG_ENERGY_STATUS       0x00000611 /* Package energy status */
#define X86_MSR_PKG_POWER_INFO          0x00000614 /* Package power range info */
#define X86_MSR_DRAM_POWER_LIMIT        0x00000618 /* DRAM RAPL power limit control */
#define X86_MSR_DRAM_ENERGY_STATUS      0x00000619 /* DRAM energy status */
#define X86_MSR_PP0_POWER_LIMIT         0x00000638 /* PP0 RAPL power limit control */
#define X86_MSR_PP0_ENERGY_STATUS       0x00000639 /* PP0 energy status */
#define X86_MSR_PP1_POWER_LIMIT         0x00000640 /* PP1 RAPL power limit control */
#define X86_MSR_PP1_ENERGY_STATUS       0x00000641 /* PP1 energy status */
#define X86_MSR_PLATFORM_ENERGY_COUNTER 0x0000064d /* Platform energy counter */
#define X86_MSR_PLATFORM_POWER_LIMIT    0x0000065c /* Platform power limit control */

/* EFLAGS/RFLAGS */
#define X86_FLAGS_CF                    (1U<<0)
#define X86_FLAGS_PF                    (1U<<2)
#define X86_FLAGS_AF                    (1U<<4)
#define X86_FLAGS_ZF                    (1U<<6)
#define X86_FLAGS_SF                    (1U<<7)
#define X86_FLAGS_TF                    (1U<<8)
#define X86_FLAGS_IF                    (1U<<9)
#define X86_FLAGS_DF                    (1U<<10)
#define X86_FLAGS_OF                    (1U<<11)
#define X86_FLAGS_STATUS_MASK           (0xfff)
#define X86_FLAGS_IOPL_MASK             (3U<<12)
#define X86_FLAGS_IOPL_SHIFT            (12)
#define X86_FLAGS_NT                    (1U<<14)
#define X86_FLAGS_RF                    (1U<<16)
#define X86_FLAGS_VM                    (1U<<17)
#define X86_FLAGS_AC                    (1U<<18)
#define X86_FLAGS_VIF                   (1U<<19)
#define X86_FLAGS_VIP                   (1U<<20)
#define X86_FLAGS_ID                    (1U<<21)
#define X86_FLAGS_RESERVED_ONES         (0x2)
#define X86_FLAGS_RESERVED              (0xffc0802a)
#define X86_FLAGS_USER                  (X86_FLAGS_CF | \
                                         X86_FLAGS_PF | \
                                         X86_FLAGS_AF | \
                                         X86_FLAGS_ZF | \
                                         X86_FLAGS_SF | \
                                         X86_FLAGS_TF | \
                                         X86_FLAGS_DF | \
                                         X86_FLAGS_OF | \
                                         X86_FLAGS_NT | \
                                         X86_FLAGS_AC | \
                                         X86_FLAGS_ID)

static inline void x86_clts(void) { __asm__ __volatile__("clts"); }
static inline void x86_hlt(void) { __asm__ __volatile__("hlt"); }
static inline void x86_sti(void) { __asm__ __volatile__("sti"); }
static inline void x86_cli(void) { __asm__ __volatile__("cli"); }
static inline void x86_ltr(uint16_t sel) { __asm__ __volatile__("ltr %%ax" ::"a"(sel)); }
static inline void x86_lidt(uintptr_t base) { __asm volatile("lidt (%0)" ::"r"(base) : "memory"); }
static inline void x86_lgdt(uintptr_t base) { __asm volatile("lgdt (%0)" ::"r"(base) : "memory"); }

static inline ulong x86_get_cr0(void) {
  ulong rv;

  __asm__ __volatile__("mov %%cr0, %0 \n\t" : "=r"(rv));
  return rv;
}

static inline void x86_set_cr0(ulong in_val) {
  __asm__ __volatile__("mov %0,%%cr0 \n\t" : : "r"(in_val));
}

static inline void set_in_cr0(ulong mask) {
    x86_set_cr0(x86_get_cr0() | mask);
}

static inline void clear_in_cr0(ulong mask) {
    x86_set_cr0(x86_get_cr0() & ~mask);
}

static inline ulong x86_get_cr2(void) {
  ulong rv;

  __asm__ __volatile__("mov %%cr2, %0" : "=r"(rv));

  return rv;
}

static inline ulong x86_get_cr3(void) {
  ulong rv;

  __asm__ __volatile__("mov %%cr3, %0" : "=r"(rv));
  return rv;
}

static inline void x86_set_cr3(ulong in_val) {
  __asm__ __volatile__("mov %0,%%cr3 \n\t" : : "r"(in_val));
}

static inline ulong x86_get_cr4(void) {
  ulong rv;

  __asm__ __volatile__("mov %%cr4, %0 \n\t" : "=r"(rv));
  return rv;
}

static inline void x86_set_cr4(ulong in_val) {
  __asm__ __volatile__("mov %0,%%cr4 \n\t" : : "r"(in_val));
}

#define DEFINE_REGISTER_ACCESSOR(REG)                     \
  static inline void x86_set_##REG(uint16_t value) {          \
    __asm__ volatile("mov %0, %%" #REG : : "r"(value));   \
  }                                                       \
  static inline uint16_t x86_get_##REG(void) {                    \
    uint16_t value;                                       \
    __asm__ volatile("mov %%" #REG ", %0" : "=r"(value)); \
    return value;                                         \
  }

DEFINE_REGISTER_ACCESSOR(ds)
DEFINE_REGISTER_ACCESSOR(es)
DEFINE_REGISTER_ACCESSOR(fs)
DEFINE_REGISTER_ACCESSOR(gs)

#undef DEFINE_REGISTER_ACCESSOR

static inline uint8_t inp(uint16_t _port) {
  uint8_t rv;
  __asm__ __volatile__("inb %1, %0" : "=a"(rv) : "dN"(_port));
  return (rv);
}

static inline uint16_t inpw(uint16_t _port) {
  uint16_t rv;
  __asm__ __volatile__("inw %1, %0" : "=a"(rv) : "dN"(_port));
  return (rv);
}

static inline uint32_t inpd(uint16_t _port) {
  uint32_t rv;
  __asm__ __volatile__("inl %1, %0" : "=a"(rv) : "dN"(_port));
  return (rv);
}

static inline void outp(uint16_t _port, uint8_t _data) {
  __asm__ __volatile__("outb %1, %0" : : "dN"(_port), "a"(_data));
}

static inline void outpw(uint16_t _port, uint16_t _data) {
  __asm__ __volatile__("outw %1, %0" : : "dN"(_port), "a"(_data));
}

static inline void outpd(uint16_t _port, uint32_t _data) {
  __asm__ __volatile__("outl %1, %0" : : "dN"(_port), "a"(_data));
}

static inline void inprep(uint16_t _port, uint8_t* _buffer, uint32_t _reads) {
  __asm__ __volatile__(
      "pushf \n\t"
      "cli \n\t"
      "cld \n\t"
      "rep insb \n\t"
      "popf \n\t"
      : "+D"(_buffer), "+c"(_reads)
      : "d"(_port)
      : "memory");
}

static inline void outprep(uint16_t _port, uint8_t* _buffer, uint32_t _writes) {
  __asm__ __volatile__(
      "pushf \n\t"
      "cli \n\t"
      "cld \n\t"
      "rep outsb \n\t"
      "popf \n\t"
      : "+S"(_buffer), "+c"(_writes)
      : "d"(_port));
}

static inline void inpwrep(uint16_t _port, uint16_t* _buffer, uint32_t _reads) {
  __asm__ __volatile__(
      "pushf \n\t"
      "cli \n\t"
      "cld \n\t"
      "rep insw \n\t"
      "popf \n\t"
      : "+D"(_buffer), "+c"(_reads)
      : "d"(_port)
      : "memory");
}

static inline void outpwrep(uint16_t _port, uint16_t* _buffer, uint32_t _writes) {
  __asm__ __volatile__(
      "pushf \n\t"
      "cli \n\t"
      "cld \n\t"
      "rep outsw \n\t"
      "popf \n\t"
      : "+S"(_buffer), "+c"(_writes)
      : "d"(_port));
}

static inline void inpdrep(uint16_t _port, uint32_t* _buffer, uint32_t _reads) {
  __asm__ __volatile__(
      "pushf \n\t"
      "cli \n\t"
      "cld \n\t"
      "rep insl \n\t"
      "popf \n\t"
      : "+D"(_buffer), "+c"(_reads)
      : "d"(_port)
      : "memory");
}

static inline void outpdrep(uint16_t _port, uint32_t* _buffer, uint32_t _writes) {
  __asm__ __volatile__(
      "pushf \n\t"
      "cli \n\t"
      "cld \n\t"
      "rep outsl \n\t"
      "popf \n\t"
      : "+S"(_buffer), "+c"(_writes)
      : "d"(_port));
}

static inline bool x86_is_paging_enabled(void) {
    if (x86_get_cr0() & X86_CR0_PG)
        return true;

    return false;
}

static inline bool x86_is_PAE_enabled(void) {
    if (x86_is_paging_enabled() == false)
        return false;

    if (!(x86_get_cr4() & X86_CR4_PAE))
        return false;

    return true;
}

static inline void cpuid(uint32_t leaf, uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d) {
    __asm__ __volatile__ (
        "cpuid"
        : "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d)
        : "a"(leaf), "c"(0));
}

static inline void cpuid_c(uint32_t leaf, uint32_t csel, uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d) {
    __asm__ __volatile__ (
        "cpuid"
        : "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d)
        : "a"(leaf), "c"(csel));
}

static inline uint64_t read_msr (uint32_t msr_id) {
    uint64_t msr_read_val;
    uint32_t low_val;
    uint32_t high_val;

    __asm__ __volatile__ (
        "rdmsr \n\t"
        : "=a" (low_val), "=d"(high_val)
        : "c" (msr_id));

    msr_read_val = high_val;
    msr_read_val = (msr_read_val << 32) | low_val;

    return msr_read_val;
}

static inline void write_msr (uint32_t msr_id, uint64_t msr_write_val) {
    uint32_t low_val = (uint32_t)msr_write_val;
    uint32_t high_val = (uint32_t)(msr_write_val >> 32);

    __asm__ __volatile__ (
        "wrmsr \n\t"
        : : "c" (msr_id), "a" (low_val), "d"(high_val));
}

#pragma GCC diagnostic push
/* The dereference of offset in the inline asm below generates this warning in GCC */
#pragma GCC diagnostic ignored "-Warray-bounds"
static inline uint64_t x86_read_gs_offset64(uintptr_t offset) {
  uint64_t ret;
  __asm__("movq  %%gs:%1, %0" : "=r"(ret) : "m"(*(uint64_t*)(offset)));
  return ret;
}

static inline void x86_write_gs_offset64(uintptr_t offset, uint64_t val) {
  __asm__("movq  %0, %%gs:%1" : : "ir"(val), "m"(*(uint64_t*)(offset)) : "memory");
}

static inline uint32_t x86_read_gs_offset32(uintptr_t offset) {
  uint32_t ret;
  __asm__("movl  %%gs:%1, %0" : "=r"(ret) : "m"(*(uint32_t*)(offset)));
  return ret;
}

static inline void x86_write_gs_offset32(uintptr_t offset, uint32_t val) {
  __asm__("movl   %0, %%gs:%1" : : "ir"(val), "m"(*(uint32_t*)(offset)) : "memory");
}
#pragma GCC diagnostic pop

/* cannot easily use C generics or C++ templates here, so do it the hard way */
#if __SIZEOF_POINTER__ == 8
static inline void *x86_read_gs_offset_ptr(uintptr_t offset) {
    return (void *)x86_read_gs_offset64(offset);
}
static inline void x86_write_gs_offset_ptr(uintptr_t offset, void *val) {
    x86_write_gs_offset64(offset, (uint64_t)(val));
}
#else
static inline void *x86_read_gs_offset_ptr(uintptr_t offset) {
    return (void *)x86_read_gs_offset32(offset);
}
static inline void x86_write_gs_offset_ptr(uintptr_t offset, void *val) {
    x86_write_gs_offset32(offset, (uint32_t)(val));
}
#endif

typedef ulong x86_flags_t;

static inline x86_flags_t x86_save_flags(void) {
    x86_flags_t state;

    __asm__ volatile(
        "pushf;"
        "pop %0"
        : "=rm" (state)
        :: "memory");

    return state;
}

static inline void x86_restore_flags(x86_flags_t flags) {
    __asm__ volatile(
        "push %0;"
        "popf"
        :: "g" (flags)
        : "memory", "cc");
}

static inline void tlbsync_local(vaddr_t address) {
    asm volatile("invlpg %0" :: "m"(*(uint8_t *)address));
}

void x86_early_init_percpu(void);

__END_CDECLS
