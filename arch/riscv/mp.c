/*
 * Copyright (c) 2019 Elliot Berman
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <lk/reg.h>
#include <lk/debug.h>
#include <lk/trace.h>
#include <lk/err.h>

#include <arch/ops.h>
#include <arch/mp.h>

#define LOCAL_TRACE 0

int hart_cpu_map[SMP_MAX_CPUS] = { [0 ... SMP_MAX_CPUS-1] = -1 };
static mp_ipi_t ipi_data[SMP_MAX_CPUS];

extern void clint_ipi_send(unsigned long target_hart);
extern void clint_ipi_clear(unsigned long target_hart);

status_t arch_mp_send_ipi(mp_cpu_mask_t target, mp_ipi_t ipi) {
	LTRACEF("target 0x%x, ipi %u\n", target, ipi);

	unsigned long hart_mask = 0;
	unsigned long c = 0, h;
	mp_cpu_mask_t m = target;
	for (; c < SMP_MAX_CPUS && m; c++, m >>= 1) {
		h = hart_cpu_map[c];
		if (m & 1) {
			hart_mask |= (1 << h);
		}
	}

	asm volatile("	fence iorw,iorw");
#if RISCV_M_MODE
	clint_send_ipis(&hart_mask);
#else
	sbi_send_ipis(&hart_mask);
#endif

	return NO_ERROR;
}

enum handler_return riscv_software_exception(void) {
#if RISCV_M_MODE
	clint_ipi_clear(riscv_current_hart());
#else
	sbi_clear_ipi();
#endif
	asm volatile("	fence ir,ir");
	mp_ipi_t reason = ipi_data[riscv_current_hart()];
	ipi_data[riscv_current_hart()] = 0;
	switch (reason) {
		case MP_IPI_RESCHEDULE:
			return INT_RESCHEDULE;
		case MP_IPI_GENERIC:
			break;
		default:
			TRACEF("unhandled ipi cause %#x, hartid %#x\n", reason, riscv_current_hart());
			panic("stopping");
			break;
	}

	return INT_NO_RESCHEDULE;

}

void arch_mp_init_percpu(void) {
	dprintf(INFO, "\nRISCV: Booting hart%d (cpu%d)\n", riscv_current_hart(), arch_curr_cpu_num());
	riscv_csr_set(RISCV_CSR_XIE, RISCV_CSR_XIE_SIE);
}
