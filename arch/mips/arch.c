/*
 * Copyright (c) 2015 Travis Geiselbrecht
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
#include <trace.h>
#include <debug.h>
#include <stdint.h>
#include <bits.h>
#include <arch/mips.h>
#include <platform.h>

#define LOCAL_TRACE 0

void arch_early_init(void)
{
    LTRACE;

    /* configure the vector table */
    uint32_t temp = mips_read_c0_status();
    temp &= ~(1<<22); /* unset BEV, which moves vectors to 0x80000000 */
    temp &= ~(1<<2);  /* clear ERL */

    /* mask all of the irq handlers */
    temp &= ~(1<<8); // IM0
    temp &= ~(1<<9); // IM1
    temp &= ~(1<<10); // IM2
    temp &= ~(1<<11); // IM3
    temp &= ~(1<<12); // IM4
    temp &= ~(1<<13); // IM5
    temp &= ~(1<<14); // IM6
    temp &= ~(1<<15); // IM7
    temp &= ~(1<<16); // IM8
    temp &= ~(1<<18); // IM9 (note the bit gap)

    mips_write_c0_status(temp);

    /* set ebase */
    mips_write_c0_ebase(MEMBASE);

    /* make sure we take exceptions in 32bit mips mode */
    mips_write_c0_config3(mips_read_c0_config3() & ~(1<<16));

    /* set vectored mode */
    temp = mips_read_c0_intctl();
    temp &= ~(0b1111 << 5);
    temp |= 1 << 5; /* 32 byte spacing */
    STATIC_ASSERT(VECTORED_OFFSET_SHIFT == 32);

    mips_write_c0_intctl(temp);

    temp = mips_read_c0_cause();
    temp |= (1<<23); /* IV vectored mode */
    mips_write_c0_cause(temp);
}

void arch_init(void)
{
    LTRACE;

    printf("MIPS registers:\n");
    printf("\tPRId 0x%x\n", mips_read_c0_prid());
    printf("\tconfig  0x%x\n", mips_read_c0_config());
    printf("\tconfig1 0x%x\n", mips_read_c0_config1());
    printf("\tconfig2 0x%x\n", mips_read_c0_config2());
    printf("\tconfig3 0x%x\n", mips_read_c0_config3());
    printf("\tconfig4 0x%x\n", mips_read_c0_config4());
    printf("\tconfig5 0x%x\n", mips_read_c0_config5());
    printf("\tconfig6 0x%x\n", mips_read_c0_config6());
    printf("\tconfig7 0x%x\n", mips_read_c0_config7());
    printf("\tstatus  0x%x\n", mips_read_c0_status());
    uint32_t intctl = mips_read_c0_intctl();
    printf("\tintctl  0x%x\n", intctl);
    printf("\t\tIPTI  0x%lx\n", BITS_SHIFT(intctl, 31, 29));
    printf("\t\tIPPCI 0x%lx\n", BITS_SHIFT(intctl, 28, 26));
    printf("\t\tIPFDC 0x%lx\n", BITS_SHIFT(intctl, 25, 23));
    printf("\tsrsctl  0x%x\n", mips_read_c0_srsctl());
    printf("\tebase   0x%x\n", mips_read_c0_ebase());
    printf("\tcount   0x%x\n", mips_read_c0_count());
    printf("\tcompare 0x%x\n", mips_read_c0_compare());

    __asm__ volatile("syscall");

    LTRACE_EXIT;
}

void arch_idle(void)
{
    asm volatile("wait");
}

void arch_chain_load(void *entry, ulong arg0, ulong arg1, ulong arg2, ulong arg3)
{
    PANIC_UNIMPLEMENTED;
}

void mips_enable_irq(uint num)
{
    uint32_t temp = mips_read_c0_status();
    if (num < 9) {
        temp |= (1 << (num + 8));
    } else if (num == 9) {
        temp |= (1 << 18);
    }
    mips_write_c0_status(temp);
}

void mips_disable_irq(uint num)
{
    uint32_t temp = mips_read_c0_status();
    if (num < 9) {
        temp &= ~(1 << (num + 8));
    } else if (num == 9) {
        temp &= ~(1 << 18);
    }
    mips_write_c0_status(temp);
}

/* unimplemented cache operations */
void arch_disable_cache(uint flags) { PANIC_UNIMPLEMENTED; }
void arch_enable_cache(uint flags) { PANIC_UNIMPLEMENTED; }

void arch_clean_cache_range(addr_t start, size_t len) { PANIC_UNIMPLEMENTED; }
void arch_clean_invalidate_cache_range(addr_t start, size_t len) { PANIC_UNIMPLEMENTED; }
void arch_invalidate_cache_range(addr_t start, size_t len) { PANIC_UNIMPLEMENTED; }
void arch_sync_cache_range(addr_t start, size_t len) { PANIC_UNIMPLEMENTED; }
