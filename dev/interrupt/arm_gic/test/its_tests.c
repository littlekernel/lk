/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

// LPI delivery through the GICv3 ITS, without a device: the ITS INT command raises an LPI as
// if its device had written its message. Each test passes trivially, saying so, when there is
// no ITS (a GICv2, or a GICv3 without one).

#include <dev/interrupt/arm_gic.h>
#include <kernel/event.h>
#include <kernel/mp.h>
#include <kernel/thread.h>
#include <lib/unittest.h>
#include <lk/err.h>
#include <platform/interrupts.h>
#include <stdio.h>

#include "../gic_v3_its.h"
#include "../gic_v3_lpi.h"

// a device id no PCI function can have here (bus 0xff is never populated) but inside the
// device table, which covers all 16 bit ids
#define TEST_DEVICE_ID 0xfff8

static volatile uint lpi_count;
static volatile uint lpi_cpu;
static event_t lpi_event = EVENT_INITIAL_VALUE(lpi_event, false, EVENT_FLAG_AUTOUNSIGNAL);

static enum handler_return lpi_handler(void *arg) {
    lpi_count++;
    lpi_cpu = arch_curr_cpu_num();
    event_signal(&lpi_event, false);
    return INT_RESCHEDULE;
}

struct test_lpi {
    uint vector;
    uint64_t addr;
    uint32_t data;
};

static bool setup_lpi(struct test_lpi *t) {
    BEGIN_TEST;
    lpi_count = 0;
    event_unsignal(&lpi_event);
    ASSERT_EQ(NO_ERROR, arm_gic_msi_alloc(arm_gicv3_its_paddr(), TEST_DEVICE_ID, &t->vector,
                                          &t->addr, &t->data), "alloc an msi");
    EXPECT_GE(t->vector, (uint)GIC_LPI_BASE, "vector is an lpi");
    EXPECT_EQ(arm_gicv3_its_paddr() + 0x10040, t->addr, "message goes to GITS_TRANSLATER");
    register_int_handler_msi(t->vector, lpi_handler, NULL, true);
    END_TEST;
}

static void teardown_lpi(struct test_lpi *t) {
    mask_interrupt(t->vector);
    register_int_handler(t->vector, NULL, NULL);
    arm_gic_msi_free(arm_gicv3_its_paddr(), TEST_DEVICE_ID, t->vector);
}

static bool its_lpi_delivery(void) {
    BEGIN_TEST;
    if (!arm_gicv3_its_ready()) {
        printf("no GICv3 ITS, skipping\n");
        END_TEST;
    }

    struct test_lpi t;
    ASSERT_TRUE(setup_lpi(&t), "set up the lpi");
    EXPECT_EQ(NO_ERROR, unmask_interrupt(t.vector), "unmask");

    EXPECT_EQ(NO_ERROR, arm_gicv3_its_inject(t.vector), "inject");
    EXPECT_EQ(NO_ERROR, event_wait_timeout(&lpi_event, 1000), "lpi delivered");
    EXPECT_EQ(1u, lpi_count, "delivery count");
    EXPECT_EQ(0u, lpi_cpu, "new LPIs go to cpu 0's collection");

    // and again, the same event
    EXPECT_EQ(NO_ERROR, arm_gicv3_its_inject(t.vector), "inject");
    EXPECT_EQ(NO_ERROR, event_wait_timeout(&lpi_event, 1000), "lpi delivered");
    EXPECT_EQ(2u, lpi_count, "delivery count");

    teardown_lpi(&t);
    END_TEST;
}

static bool its_lpi_mask(void) {
    BEGIN_TEST;
    if (!arm_gicv3_its_ready()) {
        printf("no GICv3 ITS, skipping\n");
        END_TEST;
    }

    struct test_lpi t;
    ASSERT_TRUE(setup_lpi(&t), "set up the lpi");

    // a new LPI starts masked: raising it leaves it pending
    EXPECT_EQ(NO_ERROR, arm_gicv3_its_inject(t.vector), "inject");
    EXPECT_EQ(ERR_TIMED_OUT, event_wait_timeout(&lpi_event, 20), "nothing delivered while masked");
    EXPECT_EQ(0u, lpi_count, "delivery count");

    // unmasking delivers the pending one
    EXPECT_EQ(NO_ERROR, unmask_interrupt(t.vector), "unmask");
    EXPECT_EQ(NO_ERROR, event_wait_timeout(&lpi_event, 1000), "lpi delivered");
    EXPECT_EQ(1u, lpi_count, "delivery count");

    // masked again, nothing arrives
    EXPECT_EQ(NO_ERROR, mask_interrupt(t.vector), "mask");
    EXPECT_EQ(NO_ERROR, arm_gicv3_its_inject(t.vector), "inject");
    EXPECT_EQ(ERR_TIMED_OUT, event_wait_timeout(&lpi_event, 20), "nothing delivered while masked");
    EXPECT_EQ(1u, lpi_count, "delivery count");

    EXPECT_EQ(NO_ERROR, unmask_interrupt(t.vector), "unmask");
    EXPECT_EQ(NO_ERROR, event_wait_timeout(&lpi_event, 1000), "lpi delivered");
    EXPECT_EQ(2u, lpi_count, "delivery count");

    teardown_lpi(&t);
    END_TEST;
}

// every cpu with LPIs has its own collection: move the LPI to each and see it arrive there
static bool its_lpi_move(void) {
    BEGIN_TEST;
    if (!arm_gicv3_its_ready()) {
        printf("no GICv3 ITS, skipping\n");
        END_TEST;
    }

    struct test_lpi t;
    ASSERT_TRUE(setup_lpi(&t), "set up the lpi");
    EXPECT_EQ(NO_ERROR, unmask_interrupt(t.vector), "unmask");

    uint expected = 0;
    for (uint cpu = 0; cpu < SMP_MAX_CPUS; cpu++) {
        if (!mp_is_cpu_active(cpu) || !arm_gicv3_lpi_cpu_enabled(cpu)) {
            continue;
        }
        EXPECT_EQ(NO_ERROR, arm_gicv3_its_move(t.vector, cpu), "move");
        EXPECT_EQ(NO_ERROR, arm_gicv3_its_inject(t.vector), "inject");
        EXPECT_EQ(NO_ERROR, event_wait_timeout(&lpi_event, 1000), "lpi delivered");
        EXPECT_EQ(++expected, lpi_count, "delivery count");
        EXPECT_EQ(cpu, lpi_cpu, "delivered on the expected cpu");
    }
    EXPECT_GE(expected, 1u, "at least one cpu takes lpis");

    // back to cpu 0 for whoever allocates it next
    EXPECT_EQ(NO_ERROR, arm_gicv3_its_move(t.vector, 0), "move");
    teardown_lpi(&t);
    END_TEST;
}

BEGIN_TEST_CASE(gic_its)
RUN_TEST(its_lpi_delivery)
RUN_TEST(its_lpi_mask)
RUN_TEST(its_lpi_move)
END_TEST_CASE(gic_its)
