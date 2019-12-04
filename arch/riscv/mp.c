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

#include <arch/mp.h>

#define LOCAL_TRACE 0

static mp_ipi_t ipi_data[SMP_MAX_CPUS];

extern void clint_ipi_send(unsigned long target_hart);
extern void clint_ipi_clear(unsigned long target_hart);

status_t arch_mp_send_ipi(mp_cpu_mask_t target, mp_ipi_t ipi) {
	LTRACEF("target 0x%x, ipi %u\n", target, ipi);

	unsigned long cur_hart = riscv_csr_read(mhartid);
	unsigned long h = 0;
	mp_cpu_mask_t m = target;
	for (; h < SMP_MAX_CPUS && m; h++, m >>= 1) {
		if ((m & 1) && (h != cur_hart)) {
			ipi_data[h] = ipi;
			asm volatile("	fence iorw,iorw");
			clint_ipi_send(h);
		}
	}

	if(target & (1 << cur_hart)) {
		clint_ipi_send(cur_hart);
	}

	return NO_ERROR;
}

enum handler_return riscv_software_exception(void) {
	clint_ipi_clear(riscv_csr_read(mhartid));
	asm volatile("	fence ir,ir");
	mp_ipi_t reason = ipi_data[riscv_current_hart()];
	ipi_data[riscv_current_hart()] = 0;
	switch (reason) {
		case MP_IPI_RESCHEDULE:
			return INT_RESCHEDULE;
		case MP_IPI_GENERIC:
			break;
		default:
			TRACEF("unhandled ipi cause %#x, hartid %#lx\n", reason, riscv_current_hart());
			panic("stopping");
			break;
	}

	return INT_NO_RESCHEDULE;

}

void arch_mp_init_percpu(void) {
	riscv_csr_set(mie, RISCV_MIE_MSIE);
}
