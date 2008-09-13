/*
 * Copyright (c) 2008 Travis Geiselbrecht
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
#include <kernel/thread.h>
#include <platform.h>
#include <platform/interrupts.h>
#include <platform/at91sam7.h>
#include <arch/arm.h>

static int do_nothing()
{
    return INT_NO_RESCHEDULE;
}

void platform_init_interrupts(void)
{
    AT91AIC *aic = AT91AIC_ADDR;
    int n;

    for(n = 0; n < 31; n++) {
        aic->SVR[n] = (unsigned) do_nothing;
    }
    aic->SPU = (unsigned) do_nothing;
}


status_t mask_interrupt(unsigned int vector)
{
    AT91AIC *aic = AT91AIC_ADDR;
    
    if(vector > 31) return ERR_INVALID_ARGS;
    
	aic->IDCR = (1 << vector);
    
    return NO_ERROR;
}

status_t unmask_interrupt(unsigned int vector)
{
    AT91AIC *aic = AT91AIC_ADDR;
    if(vector > 31) return ERR_INVALID_ARGS;

	aic->IECR = (1 << vector);
    
    return NO_ERROR;
}

void platform_irq(struct arm_iframe *frame)
{
    AT91AIC *aic = AT91AIC_ADDR;
    int_handler func;
    enum handler_return ret;
    
    inc_critical_section();

    func = (int_handler) aic->IVR;
//    dprintf("platform_irq() -> %p\n", func);

    ret = func(0);
    
    aic->EOICR = (unsigned) aic;

    if(ret == INT_RESCHEDULE) {
        thread_preempt();
    }

    dec_critical_section();
}

void platform_fiq(struct arm_iframe *frame)
{
}

void register_int_handler(unsigned int vector, int_handler handler, void *arg)
{
    AT91AIC *aic = AT91AIC_ADDR;
    if(vector > 31) return;
    aic->SVR[vector] = (unsigned) handler;
}
