/*
 * Copyright (c) 2009 Corey Tabaka
 * Copyright (c) 2015 Intel Corporation
 * Copyright (c) 2016 Travis Geiselbrecht
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
#define X86_CPUID_ADDR_WIDTH 0x80000008

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

#define X86_CR0_PE 0x00000001 /* protected mode enable */
#define X86_CR0_MP 0x00000002 /* monitor coprocessor */
#define X86_CR0_EM 0x00000004 /* emulation */
#define X86_CR0_TS 0x00000008 /* task switched */
#define X86_CR0_NE 0x00000020 /* enable x87 exception */
#define X86_CR0_WP 0x00010000 /* supervisor write protect */
#define X86_CR0_NW 0x20000000 /* not write-through */
#define X86_CR0_CD 0x40000000 /* cache disable */
#define X86_CR0_PG 0x80000000 /* enable paging */
#define X86_CR4_PAE 0x00000020 /* PAE paging */
#define X86_CR4_OSFXSR 0x00000200 /* os supports fxsave */
#define X86_CR4_OSXMMEXPT 0x00000400 /* os supports xmm exception */
#define X86_CR4_OSXSAVE 0x00040000 /* os supports xsave */
#define X86_CR4_SMEP 0x00100000 /* SMEP protection enabling */
#define X86_CR4_SMAP 0x00200000 /* SMAP protection enabling */
#define x86_EFER_NXE 0x00000800 /* to enable execute disable bit */
#define x86_MSR_EFER 0xc0000080 /* EFER Model Specific Register id */
#define X86_CR4_PSE 0xffffffef /* Disabling PSE bit in the CR4 */

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
      :
      : "d"(_port), "D"(_buffer), "c"(_reads));
}

static inline void outprep(uint16_t _port, uint8_t* _buffer, uint32_t _writes) {
  __asm__ __volatile__(
      "pushf \n\t"
      "cli \n\t"
      "cld \n\t"
      "rep outsb \n\t"
      "popf \n\t"
      :
      : "d"(_port), "S"(_buffer), "c"(_writes));
}

static inline void inpwrep(uint16_t _port, uint16_t* _buffer, uint32_t _reads) {
  __asm__ __volatile__(
      "pushf \n\t"
      "cli \n\t"
      "cld \n\t"
      "rep insw \n\t"
      "popf \n\t"
      :
      : "d"(_port), "D"(_buffer), "c"(_reads));
}

static inline void outpwrep(uint16_t _port, uint16_t* _buffer, uint32_t _writes) {
  __asm__ __volatile__(
      "pushf \n\t"
      "cli \n\t"
      "cld \n\t"
      "rep outsw \n\t"
      "popf \n\t"
      :
      : "d"(_port), "S"(_buffer), "c"(_writes));
}

static inline void inpdrep(uint16_t _port, uint32_t* _buffer, uint32_t _reads) {
  __asm__ __volatile__(
      "pushf \n\t"
      "cli \n\t"
      "cld \n\t"
      "rep insl \n\t"
      "popf \n\t"
      :
      : "d"(_port), "D"(_buffer), "c"(_reads));
}

static inline void outpdrep(uint16_t _port, uint32_t* _buffer, uint32_t _writes) {
  __asm__ __volatile__(
      "pushf \n\t"
      "cli \n\t"
      "cld \n\t"
      "rep outsl \n\t"
      "popf \n\t"
      :
      : "d"(_port), "S"(_buffer), "c"(_writes));
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

static inline uint32_t x86_get_address_width(void) {
    uint32_t a, b, c, d;

    cpuid(X86_CPUID_ADDR_WIDTH, &a, &b, &c, &d);

    /* Extracting bit 15:8 from eax register */
    return ((a >> 8) & 0x0ff);
}

static inline bool check_smep_avail(void) {
    uint32_t a, b, c, d;

    cpuid_c(0x7, 0, &a, &b, &c, &d);

    return ((b >> 0x06) & 0x1);
}

static inline bool check_smap_avail(void) {
    uint32_t a, b, c, d;

    cpuid_c(0x7, 0, &a, &b, &c, &d);

    return ((b >> 0x13) & 0x1);
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
    msr_read_val = (msr_read_val << 31) | low_val;

    return msr_read_val;
}

static inline void write_msr (uint32_t msr_id, uint64_t msr_write_val) {
    uint32_t low_val = (uint32_t)msr_write_val;
    uint32_t high_val = (uint32_t)(msr_write_val >> 32);

    __asm__ __volatile__ (
        "wrmsr \n\t"
        : : "c" (msr_id), "a" (low_val), "d"(high_val));
}

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

#define rdtsc(low,high) \
     __asm__ __volatile__("rdtsc" : "=a" (low), "=d" (high))

#define rdtscl(low) \
     __asm__ __volatile__("rdtsc" : "=a" (low) : : "edx")

__END_CDECLS
