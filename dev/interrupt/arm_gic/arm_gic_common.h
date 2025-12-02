/*
 * Copyright (c) 2012-2019 LK Trusty Authors. All Rights Reserved.
 * Copyright (c) 2025 Travis Geiselbrecht
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

#include <assert.h>
#include <dev/interrupt/arm_gic.h>
#include <lk/reg.h>
#include <lk/trace.h>
#include <platform/gic.h>
#include <platform/interrupts.h>

#if ARCH_ARM
#include <arch/arm.h>
#endif
#if ARCH_ARM64
#include <arch/arm64.h>
#endif

#if ARCH_ARM
#define iframe           arm_iframe
#define IFRAME_PC(frame) ((frame)->pc)
#endif
#if ARCH_ARM64
#define iframe           arm64_iframe_short
#define IFRAME_PC(frame) ((frame)->elr)
#endif

#ifdef ARCH_ARM
/*
 * AArch32 does not have 64 bit mmio support, but the gic spec allows 32 bit
 * upper and lower access to _most_ 64 bit gic registers (not GICR_VSGIPENDR,
 * GICR_VSGIR or GITS_SGIR).
 */
/* TODO: add mmio_read32 when needed */
static inline void mmio_write64(volatile uint64_t *ptr64, uint64_t val) {
    volatile uint32_t *ptr = (volatile uint32_t *)ptr64;
    mmio_write32(ptr, (uint32_t)val);
    mmio_write32(ptr + 1, val >> 32);
}
#endif

struct arm_gic {
    int gic_revision;
    vaddr_t gicc_vaddr;
    size_t gicc_size;
    vaddr_t gicd_vaddr;
    size_t gicd_size;
    vaddr_t gicr_vaddr;
    size_t gicr_size;
    size_t gicr_cpu_stride;
};

// TODO: support multiple GICs, which only really makes sense for GICv3
#define NUM_ARM_GICS 1
extern struct arm_gic arm_gics[NUM_ARM_GICS];

struct int_handler_struct {
    int_handler handler;
    void *arg;
};

// Shared common routines in arm_gic.c
struct int_handler_struct *get_int_handler(unsigned int vector, uint cpu);
status_t gic_configure_interrupt(unsigned int vector,
                                 enum interrupt_trigger_mode tm,
                                 enum interrupt_polarity pol);

/* distributor registers shared between v2 and v3 */
static inline uint32_t gicd_read(uint gic, uint32_t reg) {
    ASSERT(gic < NUM_ARM_GICS);
    volatile uint32_t *ptr = (volatile uint32_t *)(uintptr_t)(arm_gics[gic].gicd_vaddr + reg);
    return mmio_read32(ptr);
}

static inline void gicd_write(uint gic, uint32_t reg, uint32_t val) {
    ASSERT(gic < NUM_ARM_GICS);
    volatile uint32_t *ptr = (volatile uint32_t *)(uintptr_t)(arm_gics[gic].gicd_vaddr + reg);
    mmio_write32(ptr, val);
}

/* some registers of GICD are 64 bit (v3+) */
static inline uint64_t gicd_read64(uint gic, uint32_t reg) {
    ASSERT(gic < NUM_ARM_GICS);
#if !_LP64
    /* AArch32: perform two 32-bit reads, low then high */
    volatile uint32_t *ptr32 = (volatile uint32_t *)(uintptr_t)(arm_gics[gic].gicd_vaddr + reg);
    uint64_t lo = mmio_read32(ptr32);
    uint64_t hi = mmio_read32(ptr32 + 1);
    return lo | (hi << 32);
#else
    volatile uint64_t *ptr = (volatile uint64_t *)(uintptr_t)(arm_gics[gic].gicd_vaddr + reg);
    return mmio_read64(ptr);
#endif
}

static inline void gicd_write64(uint gic, uint32_t reg, uint64_t val) {
    ASSERT(gic < NUM_ARM_GICS);
#if !_LP64
    /* AArch32: perform two 32-bit writes, low then high */
    volatile uint32_t *ptr32 = (volatile uint32_t *)(uintptr_t)(arm_gics[gic].gicd_vaddr + reg);
    mmio_write32(ptr32, (uint32_t)val);
    mmio_write32(ptr32 + 1, (uint32_t)(val >> 32));
#else
    volatile uint64_t *ptr = (volatile uint64_t *)(uintptr_t)(arm_gics[gic].gicd_vaddr + reg);
    mmio_write64(ptr, val);
#endif
}

/* distribution regs */
#define GICD_CTLR          (0x000)
#define GICD_TYPER         (0x004)
#define GICD_IIDR          (0x008)
#define GICD_IGROUPR(n)    (0x080 + (n) * 4)
#define GICD_ISENABLER(n)  (0x100 + (n) * 4)
#define GICD_ICENABLER(n)  (0x180 + (n) * 4)
#define GICD_ISPENDR(n)    (0x200 + (n) * 4)
#define GICD_ICPENDR(n)    (0x280 + (n) * 4)
#define GICD_ISACTIVER(n)  (0x300 + (n) * 4)
#define GICD_ICACTIVER(n)  (0x380 + (n) * 4)
#define GICD_IPRIORITYR(n) (0x400 + (n) * 4)
#define GICD_ITARGETSR(n)  (0x800 + (n) * 4)
#define GICD_ICFGR(n)      (0xc00 + (n) * 4)
#define GICD_NSACR(n)      (0xe00 + (n) * 4)
#define GICD_SGIR          (0xf00)
#define GICD_CPENDSGIR(n)  (0xf10 + (n) * 4)
#define GICD_SPENDSGIR(n)  (0xf20 + (n) * 4)

/* GICD_CTRL  Register
 *            Non-Secure   Only_a_Single   two_Security
 * (1U << 8)  RES0         nASSGIreq       RES0
 * (1U << 7)  RES0         E1NWF           E1NWF
 * (1U << 5)  RES0         RES0            ARE_NS
 * (1U << 4)  ARE_NS       ARE             ARE_S
 * (1U << 2)  RES0         RES0            ENABLE_G1S
 * (1U << 1)  ENABLE_G1A   ENABLE_G1       ENABLE_G1NS
 * (1U << 0)  ENABLE_G1    ENABLE_G0       ENABLE_G0
 */
#define GICD_CTLR_RWP         (1U << 31)
#define GICD_CTLR_nASSGIreq   (1U << 8)
#define GICD_CTRL_E1NWF       (1U << 7)
#define GICD_CTLR_DS          (1U << 6)
#define GICD_CTLR_ARE_NS      (1U << 5)
#define GICD_CTLR_ARE_S       (1U << 4)
#define GICD_CTLR_ENABLE_G1S  (1U << 2)
#define GICD_CTLR_ENABLE_G1NS (1U << 1)
#define GICD_CTLR_ENABLE_G0   (1U << 0)

/* GICv3/v4 Distributor interface */
#define GICD_STATUSR     (0x0010)
#define GICD_SETSPI_NSR  (0x0040)
#define GICD_CLRSPI_NSR  (0x0048)
#define GICD_SETSPI_SR   (0x0050)
#define GICD_CLRSPI_SR   (0x0058)
#define GICD_IGRPMODR(n) (0x0D00 + (n) * 4)
#define GICD_IROUTER(n)  (0x6000 + (n) * 8)
#define GICD_CIDR0       (0xfff0)
#define GICD_CIDR1       (0xfff4)
#define GICD_CIDR2       (0xfff8)
#define GICD_CIDR3       (0xfffc)
#define GICD_PIDR0       (0xffe0)
#define GICD_PIDR1       (0xffe4)
#define GICD_PIDR2       (0xffe8)
#define GICD_PIDR3       (0xffec)
#define GICD_LIMIT       (0x10000)
#define GICD_MIN_SIZE    (0x10000)

/* GICv3/v4 Redistrubutor interface */

#define GICR_CPU_OFFSET(gic, cpu) ((cpu) * arm_gics[(gic)].gicr_cpu_stride)

static inline uint32_t gicr_read(uint gic, uint cpu, uint32_t reg) {
    ASSERT(gic < NUM_ARM_GICS);
    ASSERT(cpu < SMP_MAX_CPUS);
    ASSERT(arm_gics[gic].gicr_cpu_stride != 0);
    volatile uint32_t *ptr = (volatile uint32_t *)(uintptr_t)(arm_gics[gic].gicr_vaddr + GICR_CPU_OFFSET(gic, cpu) + reg);
    return mmio_read32(ptr);
}

static inline void gicr_write(uint gic, uint cpu, uint32_t reg, uint32_t val) {
    ASSERT(gic < NUM_ARM_GICS);
    ASSERT(cpu < SMP_MAX_CPUS);
    ASSERT(arm_gics[gic].gicr_cpu_stride != 0);
    volatile uint32_t *ptr = (volatile uint32_t *)(uintptr_t)(arm_gics[gic].gicr_vaddr + GICR_CPU_OFFSET(gic, cpu) + reg);
    mmio_write32(ptr, val);
}

#define GICR_CTRL    (0x0000)
#define GICR_IIDR    (0x0004)
#define GICR_TYPER   (0x0008)
#define GICR_STATUSR (0x0010)
#define GICR_WAKER   (0x0014)

/* The following GICR registers are on separate 64KB page */
#define GICR_SGI_OFFSET    (0x10000)
#define GICR_IGROUPR0      (GICR_SGI_OFFSET + 0x0080)
#define GICR_ISENABLER0    (GICR_SGI_OFFSET + 0x0100)
#define GICR_ICENABLER0    (GICR_SGI_OFFSET + 0x0180)
#define GICR_ISPENDR0      (GICR_SGI_OFFSET + 0x0200)
#define GICR_ICPENDR0      (GICR_SGI_OFFSET + 0x0280)
#define GICR_ISACTIVER0    (GICR_SGI_OFFSET + 0x0300)
#define GICR_ICACTIVER0    (GICR_SGI_OFFSET + 0x0380)
#define GICR_IPRIORITYR(n) (GICR_SGI_OFFSET + 0x0400 + (n) * 4)
#define GICR_ICFGR(n)      (GICR_SGI_OFFSET + 0x0C00 + (n) * 4)
#define GICR_IGRPMODR0     (GICR_SGI_OFFSET + 0x0D00)
#define GICR_NSACR         (GICR_SGI_OFFSET + 0x0E00)
#define GICR_LIMIT         (GICR_SGI_OFFSET + 0x1000)
#define GICR_MIN_SIZE      (0x10000)

// XXX: from trusty macros.h

#define ROUND_UP(n, d)     (((n) + (size_t)(d) - 1) & ~((size_t)(d) - 1))
#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))

static inline int arm_gic_max_cpu(void) {
    return (int)(((gicd_read(0, GICD_TYPER) >> 5) & 0x7) + 1);
}
