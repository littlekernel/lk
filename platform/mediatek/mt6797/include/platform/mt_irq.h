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
#ifndef __MT_IRQ_H__
#define __MT_IRQ_H__

#include <stdint.h>

#define GIC_DIST_CTRL           0x000
#define GIC_DIST_ENABLE_SET     0x100
#define GIC_DIST_ENABLE_CLEAR       0x180
#define GIC_DIST_PENDING_SET        0x200
#define GIC_DIST_PENDING_CLEAR      0x280
#define GIC_DIST_ACTIVE_SET        0x300
#define GIC_DIST_ACTIVE_CLEAR      0x380
#define GIC_DIST_PRI            0x400
#define GIC_DIST_CONFIG         0xc00
#define GIC_DIST_IGRPMODR       0xd00
#define GIC_DIST_ROUTE          0x6100
#define GIC_REDIS_WAKER         0x14

#define INT_POL_CTL0  (MCUCFG_BASE + 0x620)

/*
 * Define hadware registers.
 */

/*
 * Define IRQ code.
 */

#define GIC_PRIVATE_SIGNALS (32)

#define GIC_PPI_OFFSET          (27)
#define GIC_PPI_GLOBAL_TIMER    (GIC_PPI_OFFSET + 0)
#define GIC_PPI_LEGACY_FIQ      (GIC_PPI_OFFSET + 1)
#define GIC_PPI_PRIVATE_TIMER   (GIC_PPI_OFFSET + 2)
#define GIC_PPI_WATCHDOG_TIMER  (GIC_PPI_OFFSET + 3)
#define GIC_PPI_LEGACY_IRQ      (GIC_PPI_OFFSET + 4)


#define MT_GPT_IRQ_ID   201

#define MT_NR_PPI   (5)
#define MT_NR_SPI   (241)//(224)
#define NR_IRQ_LINE  (GIC_PPI_OFFSET + MT_NR_PPI + MT_NR_SPI)    // 5 PPIs and 224 SPIs

#define MT65xx_EDGE_SENSITIVE 0
#define MT65xx_LEVEL_SENSITIVE 1

#define MT65xx_POLARITY_LOW   0
#define MT65xx_POLARITY_HIGH  1

#endif  /* !__MT_IRQ_H__ */
