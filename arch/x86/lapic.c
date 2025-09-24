/*
 * Copyright (c) 2021 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include "arch/x86/apic.h"

#include <sys/types.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/reg.h>
#include <lk/trace.h>
#include <lk/init.h>
#include <lib/fixed_point.h>
#include <assert.h>
#include <kernel/thread.h>
#include <platform/interrupts.h>
#include <arch/ops.h>
#include <arch/x86.h>
#include <arch/x86/feature.h>
#include <arch/x86/mp.h>
#include <kernel/spinlock.h>
#include <platform/time.h>
#include <platform/timer.h>
#include <platform/pc/timer.h>
#include <kernel/vm.h>
#include <kernel/mp.h>

#define LOCAL_TRACE 0

static bool lapic_present = false;
static bool lapic_x2apic = false;
static bool use_tsc_deadline = false;
static volatile uint32_t *lapic_mmio;
static struct fp_32_64 timebase_to_lapic;

// TODO: move these callbacks into the shared timer code
static platform_timer_callback t_callback;
static void *callback_arg;

static void lapic_init_percpu(uint level);

// local apic registers
enum lapic_regs {
    LAPIC_ID = 0x20,
    LAPIC_VERSION = 0x30,
    LAPIC_TPR = 0x80,
    LAPIC_APR = 0x90,
    LAPIC_PPR = 0xa0,
    LAPIC_EOI = 0xb0,
    LAPIC_RRD = 0xc0,
    LAPIC_LDR = 0xd0,
    LAPIC_DFR = 0xe0,
    LAPIC_SVR = 0xf0,
    LAPIC_ISR0 = 0x100,

    LAPIC_TMR0 = 0x180,

    LAPIC_IRR0 = 0x200,

    LAPIC_ESR = 0x280,

    LAPIC_CMCI = 0x2f0,
    LAPIC_ICRLO = 0x300,
    LAPIC_ICRHI = 0x310,
    LAPIC_TIMER = 0x320,
    LAPIC_THERMAL = 0x330,
    LAPIC_PERF = 0x340,
    LAPIC_LINT0 = 0x350,
    LAPIC_LINT1 = 0x360,
    LAPIC_ERROR = 0x370,
    LAPIC_TICR = 0x380,
    LAPIC_TCCR = 0x390,
    LAPIC_DIV = 0x3e0,

    // Extended features
    LAPIC_EXT_FEATURES = 0x400,
    LAPIC_EXT_CONTROL = 0x410,
    LAPIC_EXT_SEOI = 0x420,
    LAPIC_EXT_IER0 = 0x480,
    LAPIC_EXT_LVT0 = 0x500,
};

enum lapic_interrupts {
    LAPIC_INT_TIMER = 0xf8,
    LAPIC_INT_GENERIC,
    LAPIC_INT_RESCHEDULE,

    LAPIC_INT_SPURIOUS = 0xff, // Bits 0-3 must be 1 for P6 and below compatibility
};

enum lapic_timer_mode {
    LAPIC_TIMER_MODE_ONESHOT = 0,
    LAPIC_TIMER_MODE_PERIODIC = 1,
    LAPIC_TIMER_MODE_TSC_DEADLINE = 2,
};

static uint32_t lapic_read(enum lapic_regs reg) {
    LTRACEF_LEVEL(2, "reg %#x\n", reg);
    if (lapic_x2apic) {
        // TODO: do we need barriers here?
        DEBUG_ASSERT(reg != LAPIC_ICRLO && reg != LAPIC_ICRHI);
        return read_msr(X86_MSR_IA32_X2APIC_BASE + reg / 0x10);
    } else {
        return mmio_read32(lapic_mmio + reg / 4);
    }
}

static void lapic_write(enum lapic_regs reg, uint32_t val) {
    LTRACEF_LEVEL(2, "reg %#x val %#x\n", reg, val);
    if (lapic_x2apic) {
        DEBUG_ASSERT(reg != LAPIC_ICRLO && reg != LAPIC_ICRHI);
        write_msr(X86_MSR_IA32_X2APIC_BASE + reg / 0x10, val);
    } else {
        mmio_write32(lapic_mmio + reg / 4, val);
    }
}

static void lapic_wait_for_icr_delivery(void) {
    LTRACEF_LEVEL(2, "waiting for icr\n");
    uint32_t val;
    do {
        if (lapic_x2apic) {
            val = read_msr(X86_MSR_IA32_X2APIC_BASE + 0x30);
        } else {
            val = lapic_read(LAPIC_ICRLO);
        }
    } while (val & (1u << 12));
}

// special case to write to the ICR register
static void lapic_write_icr(uint32_t low, uint32_t apic_id) {
    LTRACEF_LEVEL(2, "%#x apic_id %#x\n", low, apic_id);
    if (lapic_x2apic) {
        write_msr(X86_MSR_IA32_X2APIC_BASE + 0x30, ((uint64_t)apic_id << 32) | low);
    } else {
        lapic_write(LAPIC_ICRHI, apic_id << 24);
        lapic_write(LAPIC_ICRLO, low);
        lapic_wait_for_icr_delivery();
    }
}

status_t lapic_set_oneshot_timer(platform_timer_callback callback, void *arg, lk_time_t interval) {
    LTRACEF("cpu %u interval %u\n", arch_curr_cpu_num(), interval);

    DEBUG_ASSERT(arch_ints_disabled());

    t_callback = callback;
    callback_arg = arg;

    if (use_tsc_deadline) {
        uint64_t now = __builtin_ia32_rdtsc();
        uint64_t delta = time_to_tsc_ticks(interval);
        uint64_t deadline = now + delta;
        LTRACEF("now %llu delta %llu deadline %llu\n", now, delta, deadline);
        write_msr(X86_MSR_IA32_TSC_DEADLINE, deadline);
    } else {
        // set the initial count, which should trigger the timer
        uint64_t ticks = u64_mul_u32_fp32_64(interval, timebase_to_lapic);
        if (ticks > UINT32_MAX) {
            ticks = UINT32_MAX;
        }

        lapic_write(LAPIC_TICR, ticks & 0xffffffff);
    }

    return NO_ERROR;
}

void lapic_cancel_timer(void) {
    LTRACE;

    DEBUG_ASSERT(arch_ints_disabled());

    if (use_tsc_deadline) {
        write_msr(X86_MSR_IA32_TSC_DEADLINE, 0);
    } else {
        lapic_write(LAPIC_TICR, 0);
    }
}

static enum handler_return lapic_timer_handler(void *arg) {
    LTRACEF("cpu %u\n", arch_curr_cpu_num());

    enum handler_return ret = INT_NO_RESCHEDULE;
    if (t_callback) {
        ret = t_callback(callback_arg, current_time());
    }

    return ret;
}

static enum handler_return lapic_spurious_handler(void *arg)  {
    LTRACEF("cpu %u, arg %p\n", arch_curr_cpu_num(), arg);

    return INT_NO_RESCHEDULE;
}

static enum handler_return lapic_generic_handler(void *arg)  {
    LTRACEF("cpu %u, arg %p\n", arch_curr_cpu_num(), arg);

    return INT_NO_RESCHEDULE;
}

static enum handler_return lapic_reschedule_handler(void *arg)  {
    LTRACEF("cpu %u, arg %p\n", arch_curr_cpu_num(), arg);

    return mp_mbx_reschedule_irq();
}

void lapic_init(void) {
    lapic_present = x86_feature_test(X86_FEATURE_APIC);
}

// run on the boot cpu after vm is initialized
void lapic_init_postvm(void) {
    if (!lapic_present) {
        return;
    }

    dprintf(INFO, "X86: local apic detected\n");

    // IA32_APIC_BASE_MSR
    uint64_t apic_base = read_msr(X86_MSR_IA32_APIC_BASE);
    LTRACEF("raw apic base msr %#llx\n", apic_base);

    // check for X2APIC feature
    if (x86_feature_test(X86_FEATURE_X2APIC)) {
        lapic_x2apic = true;
        dprintf(INFO, "X86: local apic supports x2APIC mode\n");
    }

    dprintf(INFO, "X86: lapic physical address %#llx\n", apic_base & ~0xfff);

    // make sure the apic is enabled on the first cpu
    lapic_enable_on_local_cpu();

    // map the lapic into the kernel since it's not guaranteed that the physmap covers it
    if (!lapic_x2apic) {
        LTRACEF("mapping lapic into kernel\n");
        status_t err = vmm_alloc_physical(vmm_get_kernel_aspace(), "lapic", PAGE_SIZE, (void **)&lapic_mmio, 0,
                                apic_base & ~0xfff, /* vmm_flags */ 0, ARCH_MMU_FLAG_UNCACHED_DEVICE);
        ASSERT(err == NO_ERROR);
        ASSERT(lapic_mmio != NULL);
    }

    // semi-hack: re-read the APIC id of the boot cpu make sure the pecpu struct is correct
    // before we go any further, in case the boot cpu's apic id is not 0.
    x86_percpu_t *percpu = x86_get_percpu();
    percpu->apic_id = lapic_get_apic_id();
    dprintf(INFO, "X86: boot cpu apic id %u\n", percpu->apic_id);

    // Read the local apic id and version and features
    uint32_t id = lapic_read(LAPIC_ID);
    uint32_t version = lapic_read(LAPIC_VERSION);
    bool eas = version & (1u<<31);
    uint32_t max_lvt = (version >> 16) & 0xff;
    version &= 0xff;
    dprintf(INFO, "X86: local apic id %#x version %#x\n", id, version);
    dprintf(INFO, "X86: local apic max lvt entries %u\n", max_lvt);
    if (eas) {
        dprintf(INFO, "X86: local apic EAS features %#x\n", lapic_read(LAPIC_EXT_FEATURES));
    }

    // Finish up some local initialization that all cpus will want to do
    lapic_init_percpu(0);
}

static void lapic_init_percpu(uint level) {
    // If we're on a secondary cpu we should have a local apic detected and present
    DEBUG_ASSERT(lapic_present);

    lapic_enable_on_local_cpu();

    // set the spurious vector register
    uint32_t svr = (LAPIC_INT_SPURIOUS | (1u<<8)); // enable
    lapic_write(LAPIC_SVR, svr);

    LTRACEF("lapic svr %#x\n", lapic_read(LAPIC_SVR));

    register_int_handler_msi(LAPIC_INT_SPURIOUS, &lapic_spurious_handler, NULL, false);
    register_int_handler_msi(LAPIC_INT_GENERIC, &lapic_generic_handler, NULL, false);
    register_int_handler_msi(LAPIC_INT_RESCHEDULE, &lapic_reschedule_handler, NULL, false);
}
LK_INIT_HOOK_FLAGS(lapic_init_percpu, lapic_init_percpu, LK_INIT_LEVEL_VM, LK_INIT_FLAG_SECONDARY_CPUS);

void lapic_enable_on_local_cpu(void) {
    DEBUG_ASSERT(lapic_present);

    // Make sure the apic is enabled and x2apic mode is set (if supported)
    uint64_t apic_base = read_msr(X86_MSR_IA32_APIC_BASE);
    apic_base |= (1u<<11);
    if (lapic_x2apic) {
        apic_base |= (1u<<10);
    }
    write_msr(X86_MSR_IA32_APIC_BASE, apic_base);
}

uint32_t lapic_get_apic_id(void) {
    if (!lapic_present) {
        return -1;
    }

    if (lapic_x2apic) {
        return (uint32_t)read_msr(0x802);
    } else {
        return lapic_read(LAPIC_ID) >> 24;
    }
}

static uint32_t lapic_read_current_tick(void) {
    if (!lapic_present) {
        return 0;
    }

    return lapic_read(LAPIC_TCCR);
}

static void lapic_timer_init_percpu(uint level) {
    // check for deadline mode
    if (use_tsc_deadline) {
        // put the timer in TSC deadline and clear the match register
        uint32_t val = (LAPIC_TIMER_MODE_TSC_DEADLINE << 17) | LAPIC_INT_TIMER;
        lapic_write(LAPIC_TIMER, val);
        write_msr(X86_MSR_IA32_TSC_DEADLINE, 0);
    } else {
        // configure the local timer and make sure it is not set to fire
        uint32_t val = (LAPIC_TIMER_MODE_ONESHOT << 17) | LAPIC_INT_TIMER;
        lapic_write(LAPIC_TIMER, val);
        lapic_write(LAPIC_TICR, 0);
    }

    // register the timer interrupt vector
    register_int_handler_msi(LAPIC_INT_TIMER, &lapic_timer_handler, NULL, false);
}
LK_INIT_HOOK_FLAGS(lapic_timer_init_percpu, lapic_timer_init_percpu, LK_INIT_LEVEL_VM + 1, LK_INIT_FLAG_SECONDARY_CPUS);

status_t lapic_timer_init(bool invariant_tsc_supported) {
    if (!lapic_present) {
        return ERR_NOT_FOUND;
    }

    // check for deadline mode
    bool tsc_deadline  = x86_feature_test(X86_FEATURE_TSC_DEADLINE);
    if (invariant_tsc_supported && tsc_deadline) {
        dprintf(INFO, "X86: local apic timer supports TSC deadline mode\n");
        use_tsc_deadline = true;
    } else {
        // configure the local timer and make sure it is not set to fire
        uint32_t val = (LAPIC_TIMER_MODE_ONESHOT << 17) | LAPIC_INT_TIMER;
        lapic_write(LAPIC_TIMER, val);

        // calibrate the timer frequency
        lapic_write(LAPIC_TICR, 0xffffffff); // countdown from the max count
        uint32_t lapic_hz = pit_calibrate_lapic(&lapic_read_current_tick);
        lapic_write(LAPIC_TICR, 0);
        printf("X86: local apic timer frequency %uHz\n", lapic_hz);

        fp_32_64_div_32_32(&timebase_to_lapic, lapic_hz, 1000);
        dprintf(INFO, "X86: timebase to local apic timer ratio %u.%08u...\n",
                timebase_to_lapic.l0, timebase_to_lapic.l32);
    }

    lapic_timer_init_percpu(0);

    return NO_ERROR;
}

void lapic_eoi(unsigned int vector) {
    LTRACEF("vector %#x\n", vector);
    if (!lapic_present) {
        return;
    }

    lapic_write(LAPIC_EOI, 0);
}

void lapic_send_init_ipi(uint32_t apic_id, bool level) {
    if (!lapic_present) {
        return;
    }

    // Level triggered mode, level according to arg, INIT delivery mode, no shorthand
    lapic_write_icr((1u << 15) | (level ? (1u << 14) : 0) | (5u << 8), apic_id);
}

void lapic_send_startup_ipi(uint32_t apic_id, uint32_t startup_vector) {
    if (!lapic_present) {
        return;
    }

    // Startup IPI, no shorthand
    lapic_write_icr((6u << 8) | (startup_vector >> 12), apic_id);
}

void lapic_send_ipi(uint32_t apic_id, mp_ipi_t ipi) {
    if (!lapic_present) {
        return;
    }

    LTRACEF("cpu %u target apic_id %#x, ipi %u\n", arch_curr_cpu_num(), apic_id, ipi);

    uint32_t vector;
    switch (ipi) {
        case MP_IPI_GENERIC:
            vector = LAPIC_INT_GENERIC;
            break;
        case MP_IPI_RESCHEDULE:
            vector = LAPIC_INT_RESCHEDULE;
            break;
        default:
            panic("X86: unknown IPI %u\n", ipi);
    }

    // send fixed mode, level asserted, no destination shorthand interrupt
    lapic_write_icr(vector | (1U << 14), apic_id);
}