/*
 * Copyright (c) 2015 MediaTek Inc.
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
#ifndef _MT_GIC_H_
#define _MT_GIC_H_

#include "platform/mt_irq.h"

#define IRQ_REGS    ((NR_IRQ_LINE + (32 - 1)) >> 5)

enum {IRQ_MASK_HEADER = 0xF1F1F1F1, IRQ_MASK_FOOTER = 0xF2F2F2F2};

struct mtk_irq_mask {
    unsigned int header;   /* for error checking */
    unsigned int mask[IRQ_REGS];
    unsigned int footer;   /* for error checking */
};

int mt_irq_mask_all(struct mtk_irq_mask *mask); //(This is ONLY used for the sleep driver)
int mt_irq_mask_restore(struct mtk_irq_mask *mask); //(This is ONLY used for the sleep driver)
void mt_irq_set_sens(unsigned int irq, unsigned int sens);
void mt_irq_set_polarity(unsigned int irq, unsigned int polarity);
void mt_irq_mask(unsigned int irq);
void mt_irq_unmask(unsigned int irq);
uint32_t mt_irq_get(void);
void mt_irq_ack(unsigned int irq);

void platform_init_interrupts(void);
void platform_deinit_interrupts(void);
void mt_irq_register_dump(void);

#endif /* !_MT_GIC_H_ */

