/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <assert.h>
#include <lk/trace.h>
#include <lk/debug.h>
#include <stdint.h>
#include <arch/riscv.h>
#include <arch/ops.h>
#include <arch/mp.h>
#include <lk/init.h>
#include <lk/main.h>
#include <platform.h>

#define LOCAL_TRACE 0

#if WITH_SMP
static spin_lock_t boot_cpu_lock = 1;
static volatile int secondaries_to_init = SMP_MAX_CPUS - 1;
#endif

void arch_early_init(void) {
    // set the top level exception handler
    riscv_csr_write(RISCV_CSR_XTVEC, (uintptr_t)&riscv_exception_entry);

    // mask all exceptions, just in case
    riscv_csr_clear(RISCV_CSR_XSTATUS, RISCV_CSR_XSTATUS_IE);
    riscv_csr_clear(RISCV_CSR_XIE, RISCV_CSR_XIE_SIE | RISCV_CSR_XIE_TIE | RISCV_CSR_XIE_EIE);

    // enable cycle counter (disabled for now, unimplemented on sifive-e)
    //riscv_csr_set(mcounteren, 1);
}

void arch_init(void) {
    // print some arch info
    dprintf(INFO, "RISCV: mvendorid %#lx marchid %#lx mimpid %#lx mhartid %#x\n",
            riscv_get_mvendorid(), riscv_get_marchid(),
            riscv_get_mimpid(), riscv_current_hart());
#if RISCV_M_MODE
    dprintf(INFO, "RISCV: misa %#lx\n", riscv_csr_read(RISCV_CSR_MISA));
#else
    dprintf(INFO, "RISCV: sbi %#lx (%#lx)\n", sbi_call(SBI_GET_SBI_IMPL_ID).value, sbi_call(SBI_GET_SBI_IMPL_VERSION).value);
#endif

    // enable external interrupts
    riscv_csr_set(RISCV_CSR_XIE, RISCV_CSR_XIE_EIE);

#if WITH_SMP
    arch_mp_init_percpu();

    lk_init_secondary_cpus(secondaries_to_init);

    LTRACEF("RISCV: Waiting for %d secondary harts to come up\n", secondaries_to_init);
    /* release the secondary cpus */
    spin_unlock(&boot_cpu_lock);
    // while (secondaries_to_init) arch_idle();
    // spin_lock(&boot_cpu_lock);
#endif
}

#if WITH_SMP
void riscv_secondary_entry(void) {
    arch_early_init();

    if (unlikely(arch_curr_cpu_num() >= SMP_MAX_CPUS))
        while (1) arch_idle();

    spin_lock(&boot_cpu_lock);
    spin_unlock(&boot_cpu_lock);

    // enable external interrupts
    riscv_csr_set(RISCV_CSR_XIE, RISCV_CSR_XIE_EIE);

    /* run early secondary cpu init routines up to the threading level */
    lk_init_level(LK_INIT_FLAG_SECONDARY_CPUS, LK_INIT_LEVEL_EARLIEST, LK_INIT_LEVEL_THREADING - 1);

    arch_mp_init_percpu();

# if RISCV_M_MODE
    dprintf(INFO, "RISCV: secondary hart coming up: mvendorid %#lx marchid %#lx mimpid %#lx mhartid %#x\n",
            riscv_csr_read(mvendorid), riscv_csr_read(marchid),
            riscv_csr_read(mimpid), riscv_current_hart());
# endif /* RISCV_M_MODE */

    // atomic_add(&secondaries_to_init, -1);
    // arch_mp_send_ipi(1 << 0, MP_IPI_GENERIC); // wake up hart0 to let it know this CPU has come up

    lk_secondary_cpu_entry();
}
#endif

void arch_idle(void) {
    // disabled for now, QEMU seems to have some trouble emulating wfi properly
    // also have trouble breaking into sifive-e board with openocd when wfi
    // NOTE: reenabling for now, will need to re-test on sifive board to see if this
    // problem went away.
    __asm__ volatile("wfi");
}

void arch_chain_load(void *entry, ulong arg0, ulong arg1, ulong arg2, ulong arg3) {
    PANIC_UNIMPLEMENTED;
}

/* unimplemented cache operations */
void arch_disable_cache(uint flags) { PANIC_UNIMPLEMENTED; }
void arch_enable_cache(uint flags) { PANIC_UNIMPLEMENTED; }

void arch_clean_cache_range(addr_t start, size_t len) { PANIC_UNIMPLEMENTED; }
void arch_clean_invalidate_cache_range(addr_t start, size_t len) { PANIC_UNIMPLEMENTED; }
void arch_invalidate_cache_range(addr_t start, size_t len) { PANIC_UNIMPLEMENTED; }
void arch_sync_cache_range(addr_t start, size_t len) { PANIC_UNIMPLEMENTED; }
