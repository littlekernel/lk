/*
 * Copyright (c) 2012-2014 Travis Geiselbrecht
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
#include <err.h>
#include <sys/types.h>
#include <debug.h>
#include <dev/interrupt/arm_gic.h>
#include <reg.h>
#include <kernel/thread.h>
#include <kernel/debug.h>
#include <platform/interrupts.h>
#include <arch/ops.h>
#include <arch/arm.h>
#include <platform/gic.h>
#include <trace.h>
#if WITH_LIB_SM
#include <lib/sm.h>
#include <lib/sm/sm_err.h>
#endif

#define LOCAL_TRACE 0

static status_t arm_gic_set_secure_locked(u_int irq, bool secure);

static spin_lock_t gicd_lock;
#if WITH_LIB_SM
#define GICD_LOCK_FLAGS SPIN_LOCK_FLAG_IRQ_FIQ
#else
#define GICD_LOCK_FLAGS SPIN_LOCK_FLAG_INTERRUPTS
#endif
#define GIC_MAX_PER_CPU_INT 32

#if WITH_LIB_SM
static bool arm_gic_non_secure_interrupts_frozen;

static bool arm_gic_interrupt_change_allowed(int irq)
{
	if (!arm_gic_non_secure_interrupts_frozen)
		return true;

	TRACEF("change to interrupt %d ignored after booting ns\n", irq);
	return false;
}
#else
static bool arm_gic_interrupt_change_allowed(int irq)
{
	return true;
}
#endif


struct int_handler_struct {
	int_handler handler;
	void *arg;
};

static struct int_handler_struct int_handler_table[MAX_INT][SMP_MAX_CPUS];

void register_int_handler(unsigned int vector, int_handler handler, void *arg)
{
	uint cpu = arch_curr_cpu_num();

	spin_lock_saved_state_t state;

	if (vector >= MAX_INT)
		panic("register_int_handler: vector out of range %d\n", vector);

	spin_lock_save(&gicd_lock, &state, GICD_LOCK_FLAGS);

	if (arm_gic_interrupt_change_allowed(vector)) {
		int_handler_table[vector][cpu].handler = handler;
		int_handler_table[vector][cpu].arg = arg;
	}

	spin_unlock_restore(&gicd_lock, state, GICD_LOCK_FLAGS);
}

#define GICREG(gic, reg) (*REG32(GICBASE(gic) + (reg)))

/* main cpu regs */
#define GICC_CTLR               (GICC_OFFSET + 0x0000)
#define GICC_PMR                (GICC_OFFSET + 0x0004)
#define GICC_BPR                (GICC_OFFSET + 0x0008)
#define GICC_IAR                (GICC_OFFSET + 0x000c)
#define GICC_EOIR               (GICC_OFFSET + 0x0010)
#define GICC_RPR                (GICC_OFFSET + 0x0014)
#define GICC_HPPIR              (GICC_OFFSET + 0x0018)
#define GICC_APBR               (GICC_OFFSET + 0x001c)
#define GICC_AIAR               (GICC_OFFSET + 0x0020)
#define GICC_AEOIR              (GICC_OFFSET + 0x0024)
#define GICC_AHPPIR             (GICC_OFFSET + 0x0028)
#define GICC_APR(n)             (GICC_OFFSET + 0x00d0 + (n) * 4)
#define GICC_NSAPR(n)           (GICC_OFFSET + 0x00e0 + (n) * 4)
#define GICC_IIDR               (GICC_OFFSET + 0x00fc)
#define GICC_DIR                (GICC_OFFSET + 0x1000)

/* distribution regs */
#define GICD_CTLR               (GICD_OFFSET + 0x000)
#define GICD_TYPER              (GICD_OFFSET + 0x004)
#define GICD_IIDR               (GICD_OFFSET + 0x008)
#define GICD_IGROUPR(n)         (GICD_OFFSET + 0x080 + (n) * 4)
#define GICD_ISENABLER(n)       (GICD_OFFSET + 0x100 + (n) * 4)
#define GICD_ICENABLER(n)       (GICD_OFFSET + 0x180 + (n) * 4)
#define GICD_ISPENDR(n)         (GICD_OFFSET + 0x200 + (n) * 4)
#define GICD_ICPENDR(n)         (GICD_OFFSET + 0x280 + (n) * 4)
#define GICD_ISACTIVER(n)       (GICD_OFFSET + 0x300 + (n) * 4)
#define GICD_ICACTIVER(n)       (GICD_OFFSET + 0x380 + (n) * 4)
#define GICD_IPRIORITYR(n)      (GICD_OFFSET + 0x400 + (n) * 4)
#define GICD_ITARGETSR(n)       (GICD_OFFSET + 0x800 + (n) * 4)
#define GICD_ICFGR(n)           (GICD_OFFSET + 0xc00 + (n) * 4)
#define GICD_NSACR(n)           (GICD_OFFSET + 0xe00 + (n) * 4)
#define GICD_SGIR               (GICD_OFFSET + 0xf00)
#define GICD_CPENDSGIR(n)       (GICD_OFFSET + 0xf10 + (n) * 4)
#define GICD_SPENDSGIR(n)       (GICD_OFFSET + 0xf20 + (n) * 4)

static void gic_set_enable(uint vector, bool enable)
{
	int reg = vector / 32;
	uint32_t mask = 1ULL << (vector % 32);

	if (enable)
		GICREG(0, GICD_ISENABLER(reg)) = mask;
	else
		GICREG(0, GICD_ICENABLER(reg)) = mask;
}

void arm_gic_init_percpu(void)
{
#if WITH_LIB_SM
	GICREG(0, GICC_CTLR) = 0xb; // enable GIC0 and select fiq mode for secure
	GICREG(0, GICD_IGROUPR(0)) = ~0UL; /* GICD_IGROUPR0 is banked */
#else
	GICREG(0, GICC_CTLR) = 1; // enable GIC0
#endif
	GICREG(0, GICC_PMR) = 0xFF; // unmask interrupts at all priority levels
}

static int arm_gic_max_cpu(void)
{
	return (GICREG(0, GICD_TYPER) >> 5) & 0x7;
}

void arm_gic_init(void)
{
	int i;

	for (i = 0; i < MAX_INT; i+= 32) {
		GICREG(0, GICD_ICENABLER(i / 32)) = ~0;
		GICREG(0, GICD_ICPENDR(i / 32)) = ~0;
	}

	if (arm_gic_max_cpu() > 0) {
		/* Set external interrupts to target cpu 0 */
		for (i = 32; i < MAX_INT; i += 4) {
			GICREG(0, GICD_ITARGETSR(i / 4)) = 0x01010101;
		}
	}

	GICREG(0, GICD_CTLR) = 1; // enable GIC0
#if WITH_LIB_SM
	/*
	 * Iterate through all IRQs and set them to non-secure
	 * mode. This will allow the non-secure side to handle
	 * all the interrupts we don't explicitly claim.
	 */
	for (i = 32; i < MAX_INT; i += 32)
		GICREG(0, GICD_IGROUPR(i / 32)) = ~0UL;
#endif
	arm_gic_init_percpu();
}

static status_t arm_gic_set_secure_locked(u_int irq, bool secure)
{
#if WITH_LIB_SM
	int reg = irq / 32;
	uint32_t mask = 1ULL << (irq % 32);

	if (irq >= MAX_INT)
		return ERR_INVALID_ARGS;

	if (secure)
		GICREG(0, GICD_IGROUPR(reg)) &= ~mask;
	else
		GICREG(0, GICD_IGROUPR(reg)) |= mask;
	LTRACEF("irq %d, secure %d, GICD_IGROUP%d = %x\n",
		irq, secure, reg, GICREG(0, GICD_IGROUPR(reg)));
#endif
	return NO_ERROR;
}

static status_t arm_gic_set_target_locked(u_int irq, u_int cpu_mask, u_int enable_mask)
{
	u_int reg = irq / 4;
	u_int shift = 8 * (irq % 4);
	u_int old_val;
	u_int new_val;

	cpu_mask = (cpu_mask & 0xff) << shift;
	enable_mask = (enable_mask << shift) & cpu_mask;

	old_val = GICREG(0, GICD_ITARGETSR(reg));
	new_val = (old_val & ~cpu_mask) | enable_mask;
	GICREG(0, GICD_ITARGETSR(reg)) = new_val;
	LTRACEF("irq %i, GICD_ITARGETSR%d %x => %x (got %x)\n",
		irq, reg, old_val, new_val, GICREG(0, GICD_ITARGETSR(reg)));

	return NO_ERROR;
}

static status_t arm_gic_get_priority(u_int irq)
{
	u_int reg = irq / 4;
	u_int shift = 8 * (irq % 4);
	return (GICREG(0, GICD_IPRIORITYR(reg)) >> shift) & 0xff;
}

static status_t arm_gic_set_priority_locked(u_int irq, uint8_t priority)
{
	u_int reg = irq / 4;
	u_int shift = 8 * (irq % 4);
	u_int mask = 0xff << shift;
	uint32_t regval;

	regval = GICREG(0, GICD_IPRIORITYR(reg));
	LTRACEF("irq %i, old GICD_IPRIORITYR%d = %x\n", irq, reg, regval);
	regval = (regval & ~mask) | ((uint32_t)priority << shift);
	GICREG(0, GICD_IPRIORITYR(reg)) = regval;
	LTRACEF("irq %i, new GICD_IPRIORITYR%d = %x, req %x\n",
		irq, reg, GICREG(0, GICD_IPRIORITYR(reg)), regval);

	return 0;
}

status_t arm_gic_sgi(u_int irq, u_int flags, u_int cpu_mask)
{
	u_int val =
		((flags & ARM_GIC_SGI_FLAG_TARGET_FILTER_MASK) << 24) |
		((cpu_mask & 0xff) << 16) |
		((flags & ARM_GIC_SGI_FLAG_NS) ? (1U << 15) : 0) |
		(irq & 0xf);

	if (irq >= 16)
		return ERR_INVALID_ARGS;

	LTRACEF("GICD_SGIR: %x\n", val);

	GICREG(0, GICD_SGIR) = val;

	return NO_ERROR;
}

status_t mask_interrupt(unsigned int vector)
{
	if (vector >= MAX_INT)
		return ERR_INVALID_ARGS;

	if (arm_gic_interrupt_change_allowed(vector))
		gic_set_enable(vector, false);

	return NO_ERROR;
}

status_t unmask_interrupt(unsigned int vector)
{
	if (vector >= MAX_INT)
		return ERR_INVALID_ARGS;

	if (arm_gic_interrupt_change_allowed(vector))
		gic_set_enable(vector, true);

	return NO_ERROR;
}

static
enum handler_return __platform_irq(struct arm_iframe *frame)
{
	// get the current vector
	uint32_t iar = GICREG(0, GICC_IAR);
	unsigned int vector = iar & 0x3ff;

	if (vector >= 0x3fe) {
		// spurious
		return INT_NO_RESCHEDULE;
	}

	THREAD_STATS_INC(interrupts);
	KEVLOG_IRQ_ENTER(vector);

	uint cpu = arch_curr_cpu_num();

//	printf("platform_irq: iar 0x%x cpu %u spsr 0x%x, pc 0x%x, currthread %p, vector %d\n",
//			iar, cpu, frame->spsr, frame->pc, get_current_thread(), vector);

	// deliver the interrupt
	enum handler_return ret;

	ret = INT_NO_RESCHEDULE;
	struct int_handler_struct *handler = &int_handler_table[vector][cpu];
	if (handler->handler)
		ret = handler->handler(handler->arg);

	GICREG(0, GICC_EOIR) = iar;

//	printf("platform_irq: cpu %u exit %d\n", cpu, ret);

	KEVLOG_IRQ_EXIT(vector);

	return ret;
}

enum handler_return platform_irq(struct arm_iframe *frame)
{
#if WITH_LIB_SM
	uint32_t ahppir = GICREG(0, GICC_AHPPIR);
	uint32_t pending_irq = ahppir & 0x3ff;

	LTRACEF("ahppir %d\n", ahppir);
	if (pending_irq < MAX_INT && int_handler_table[pending_irq].handler) {
		enum handler_return ret = 0;
		uint32_t irq;
		uint8_t old_priority;
		spin_lock_saved_state_t state;

		spin_lock_save(&gicd_lock, &state, GICD_LOCK_FLAGS);

		/* Temporarily raise the priority of the interrupt we want to
		 * handle so another interrupt does not take its place before
		 * we can acknowledge it.
		 */
		old_priority = arm_gic_get_priority(pending_irq);
		arm_gic_set_priority_locked(pending_irq, 0);
		DSB;
		irq = GICREG(0, GICC_AIAR) & 0x3ff;
		arm_gic_set_priority_locked(pending_irq, old_priority);

		spin_unlock_restore(&gicd_lock, state, GICD_LOCK_FLAGS);

		LTRACEF("irq %d\n", irq);
		if (irq < MAX_INT && int_handler_table[irq].handler)
			ret = int_handler_table[irq].handler(int_handler_table[irq].arg);
		else
			TRACEF("unexpected irq %d != %d may get lost\n", irq, pending_irq);
		GICREG(0, GICC_AEOIR) = irq;
		return ret;
	}
	return sm_handle_irq();
#else
	return __platform_irq(frame);
#endif
}

void platform_fiq(struct arm_iframe *frame)
{
#if WITH_LIB_SM
	sm_handle_irq();
#else
	PANIC_UNIMPLEMENTED;
#endif
}

#if WITH_LIB_SM
static status_t arm_gic_get_next_irq_locked(u_int min_irq, bool per_cpu)
{
	u_int irq;
	u_int max_irq = per_cpu ? GIC_MAX_PER_CPU_INT : MAX_INT;

	if (!per_cpu && min_irq < GIC_MAX_PER_CPU_INT)
		min_irq = GIC_MAX_PER_CPU_INT;

	for (irq = min_irq; irq < max_irq; irq++)
		if (int_handler_table[irq].handler)
			return irq;

	return SM_ERR_END_OF_INPUT;
}

long smc_intc_get_next_irq(smc32_args_t *args)
{
	status_t ret;
	spin_lock_saved_state_t state;

	spin_lock_save(&gicd_lock, &state, GICD_LOCK_FLAGS);

	arm_gic_non_secure_interrupts_frozen = true;
	ret = arm_gic_get_next_irq_locked(args->params[0], args->params[1]);

	spin_unlock_restore(&gicd_lock, state, GICD_LOCK_FLAGS);

	return ret;
}

long smc_intc_request_fiq(smc32_args_t *args)
{
	u_int fiq = args->params[0];
	bool enable = args->params[1];
	spin_lock_saved_state_t state;

	dprintf(SPEW, "%s: fiq %d, enable %d\n", __func__, fiq, enable);
	spin_lock_save(&gicd_lock, &state, GICD_LOCK_FLAGS);

	arm_gic_set_secure_locked(fiq, true);
	arm_gic_set_target_locked(fiq, ~0, ~0);
	arm_gic_set_priority_locked(fiq, 0);

	gic_set_enable(fiq, enable);

	dprintf(SPEW, "%s: fiq %d, enable %d done\n", __func__, fiq, enable);

	spin_unlock_restore(&gicd_lock, state, GICD_LOCK_FLAGS);

	return NO_ERROR;
}

static uint32_t read_mpidr(void)
{
	int mpidr;
	__asm__ volatile("mrc		p15, 0, %0, c0, c0, 5"
		: "=r" (mpidr)
		);
	return mpidr;
}

static u_int current_fiq[8] = { 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff };

status_t sm_intc_fiq_enter(void)
{
	u_int cpu = read_mpidr() & 7;
	u_int irq = GICREG(0, GICC_IAR) & 0x3ff;

	LTRACEF("cpu %d, irq %i\n", cpu, irq);

	if (irq >= 1020) {
		LTRACEF("spurious fiq: cpu %d, old %d, new %d\n", cpu, current_fiq[cpu], irq);
		return ERR_NO_MSG;
	}

	if (arm_gic_max_cpu() > 0) {
		spin_lock(&gicd_lock); /* IRQs and FIQs are already masked */
		arm_gic_set_target_locked(irq, 1U << cpu, 0);
		spin_unlock(&gicd_lock);
	} else {
		/* target register has no effect on uniprocessor systems */
		gic_set_enable(irq, 0);
	}
	GICREG(0, GICC_EOIR) = irq;

	if (current_fiq[cpu] != 0x3ff) {
		dprintf(INFO, "more than one fiq active: cpu %d, old %d, new %d\n", cpu, current_fiq[cpu], irq);
		return ERR_ALREADY_STARTED;
	}
	current_fiq[cpu] = irq;

	return 0;
}

void sm_intc_fiq_exit(void)
{
	u_int cpu = read_mpidr() & 7;
	LTRACEF("cpu %d, irq %i\n", cpu, current_fiq[cpu]);
	if (current_fiq[cpu] == 0x3ff) {
		dprintf(INFO, "%s: no fiq active, cpu %d\n", __func__, cpu);
		return;
	}
	if (arm_gic_max_cpu() > 0) {
		spin_lock(&gicd_lock); /* IRQs and FIQs are already masked */
		arm_gic_set_target_locked(current_fiq[cpu], 1U << cpu, ~0);
		spin_unlock(&gicd_lock);
	} else {
		gic_set_enable(current_fiq[cpu], 1);
	}
	current_fiq[cpu] = 0x3ff;
}
#endif

/* vim: set ts=4 sw=4 noexpandtab: */
