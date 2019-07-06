/*
 * Copyright (c) 2015 MediaTek Inc.
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

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

