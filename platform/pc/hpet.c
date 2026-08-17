/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <platform/pc/hpet.h>

#include <lk/debug.h>
#include <lk/err.h>
#include <lk/reg.h>
#include <lk/trace.h>
#include <sys/types.h>

#define LOCAL_TRACE 0

#if WITH_LIB_ACPI

#include <inttypes.h>
#include <kernel/vm.h>
#include <lib/acpi.h>
#include <lib/fixed_point.h>
#include <uacpi/acpi.h>

// HPET MMIO register offsets (relative to the block base address)
#define HPET_REG_CAPS         0x00 // General Capabilities and ID Register
#define HPET_REG_CONFIG       0x10 // General Configuration Register
#define HPET_REG_COUNTER_LOW  0xf0 // Main Counter Value Register, low 32 bits
#define HPET_REG_COUNTER_HIGH 0xf4 // Main Counter Value Register, high 32 bits

#define HPET_CAPS_COUNT_SIZE_CAP (1u << 13)
#define HPET_CAPS_PERIOD_SHIFT   32

#define HPET_CONFIG_ENABLE_CNF (1u << 0)

// The HPET spec caps the counter period at 100ns, i.e. the counter never ticks slower
// than 10MHz. A period outside that range means we are not looking at a sane HPET.
#define HPET_MAX_PERIOD_FS 0x05f5e100u

static volatile uint32_t *hpet_regs;
static bool hpet_counter_is_64bit;
static uint64_t hpet_hz;
static struct fp_32_64 hpet_to_ms;
static struct fp_32_64 hpet_to_us;

static uint32_t hpet_reg_read(size_t offset) {
    return mmio_read32(hpet_regs + offset / sizeof(uint32_t));
}

static void hpet_reg_write(size_t offset, uint32_t val) {
    mmio_write32(hpet_regs + offset / sizeof(uint32_t), val);
}

// Read the 64-bit main counter with a low/high/low retry to avoid tearing across
// the two 32-bit accesses, since this code has to work on non-64-bit-MMIO-capable
// builds as well.
static uint64_t hpet_read_counter(void) {
    if (!hpet_counter_is_64bit) {
        return hpet_reg_read(HPET_REG_COUNTER_LOW);
    }

    uint32_t hi, lo, hi2;
    do {
        hi = hpet_reg_read(HPET_REG_COUNTER_HIGH);
        lo = hpet_reg_read(HPET_REG_COUNTER_LOW);
        hi2 = hpet_reg_read(HPET_REG_COUNTER_HIGH);
    } while (hi != hi2);

    return ((uint64_t)hi << 32) | lo;
}

// Difference between two counter reads, masked back down to the width of the counter so
// that a single wrap during a measurement still yields the right delta. Callers are
// responsible for keeping their measurement window short enough not to wrap twice.
static uint64_t hpet_counter_delta(uint64_t start, uint64_t end) {
    const uint64_t delta = end - start;

    return hpet_counter_is_64bit ? delta : (delta & 0xffffffff);
}

lk_time_t hpet_current_time(void) {
    return u32_mul_u64_fp32_64(hpet_read_counter(), hpet_to_ms);
}

lk_bigtime_t hpet_current_time_hires(void) {
    return u64_mul_u64_fp32_64(hpet_read_counter(), hpet_to_us);
}

bool hpet_is_available(void) {
    return hpet_regs != NULL;
}

// Measure the TSC against the HPET's free-running counter.
//
// Unlike the PIT based calibration this samples both counters at the same two instants
// instead of assuming that a programmed interval elapsed, so anything that stretches the
// measurement window -- an SMI, a slow emulated register access, a preempted vcpu --
// stretches the HPET delta and the TSC delta alike and the ratio between them survives.
// That also means it does not care whether interrupts are enabled.
uint64_t hpet_calibrate_tsc(void) {
    if (!hpet_is_available()) {
        return 0;
    }

    // 10ms is long enough that the cost of a counter read is noise and short enough to
    // stay four orders of magnitude away from the ~5 minute wrap of a 32-bit counter.
    const uint64_t window = hpet_hz / 100;

    const uint64_t hpet_start = hpet_read_counter();
    const uint64_t tsc_start = __builtin_ia32_rdtsc();

    uint64_t hpet_delta;
    do {
        hpet_delta = hpet_counter_delta(hpet_start, hpet_read_counter());
    } while (hpet_delta < window);

    const uint64_t tsc_delta = __builtin_ia32_rdtsc() - tsc_start;

    if (hpet_delta == 0) {
        return 0;
    }

    // tsc_delta is tens of millions of ticks over this window and hpet_hz is tens of
    // MHz, so the product stays well inside 64 bits.
    const uint64_t tsc_freq = (tsc_delta * hpet_hz) / hpet_delta;

    dprintf(INFO, "HPET: calibrated TSC frequency: %" PRIu64 "Hz\n", tsc_freq);

    return tsc_freq;
}

status_t hpet_init(void) {
    // Callers may probe for an HPET more than once; only set one up the first time.
    if (hpet_is_available()) {
        return NO_ERROR;
    }

    const struct acpi_hpet *table =
        (const struct acpi_hpet *)acpi_get_table_by_sig(ACPI_HPET_SIGNATURE);
    if (!table) {
        LTRACEF("no HPET table found\n");
        return ERR_NOT_FOUND;
    }

    if (table->address.address_space_id != ACPI_AS_ID_SYS_MEM) {
        dprintf(INFO, "PC: HPET block is not memory mapped, unsupported\n");
        return ERR_NOT_SUPPORTED;
    }

    paddr_t phys_addr = (paddr_t)table->address.address;

    status_t err = vmm_alloc_physical(vmm_get_kernel_aspace(), "hpet", PAGE_SIZE,
                                      (void **)&hpet_regs, 0, phys_addr,
                                      /* vmm_flags */ 0, ARCH_MMU_FLAG_UNCACHED_DEVICE);
    if (err != NO_ERROR) {
        dprintf(INFO, "PC: failed to map HPET registers: %d\n", err);
        return err;
    }

    uint32_t caps_low = hpet_reg_read(HPET_REG_CAPS);
    uint32_t period_fs = hpet_reg_read(HPET_REG_CAPS + 4); // upper 32 bits of the caps register

    hpet_counter_is_64bit = (caps_low & HPET_CAPS_COUNT_SIZE_CAP) != 0;

    if (period_fs == 0 || period_fs > HPET_MAX_PERIOD_FS) {
        dprintf(INFO, "PC: HPET reports an out of spec counter period %u, ignoring\n", period_fs);
        vmm_free_region(vmm_get_kernel_aspace(), (vaddr_t)hpet_regs);
        hpet_regs = NULL;
        return ERR_NOT_SUPPORTED;
    }

    // period_fs is femtoseconds per tick, so frequency = 1e15 / period_fs. The period
    // check above keeps this at 10MHz or better, which the calibration code relies on.
    hpet_hz = 1000000000000000ULL / period_fs;

    fp_32_64_div_32_64(&hpet_to_ms, 1000, hpet_hz);
    fp_32_64_div_32_64(&hpet_to_us, 1000 * 1000, hpet_hz);

    // Start the counter running.
    uint32_t config = hpet_reg_read(HPET_REG_CONFIG);
    hpet_reg_write(HPET_REG_CONFIG, config | HPET_CONFIG_ENABLE_CNF);

    dprintf(INFO, "PC: HPET found at %#" PRIxPTR ", %sfrequency %" PRIu64 "Hz\n", phys_addr,
            hpet_counter_is_64bit ? "64-bit counter, " : "32-bit counter, ", hpet_hz);

    return NO_ERROR;
}

#else // !WITH_LIB_ACPI

status_t hpet_init(void) {
    return ERR_NOT_SUPPORTED;
}

bool hpet_is_available(void) {
    return false;
}

uint64_t hpet_calibrate_tsc(void) {
    return 0;
}

lk_time_t hpet_current_time(void) {
    return 0;
}

lk_bigtime_t hpet_current_time_hires(void) {
    return 0;
}

#endif
