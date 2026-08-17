/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

// Cross checks between the frequencies the cpu claims for itself and what the platform's
// clocks measure. The two are independent, so agreement means both are believable.

#include <arch/x86/clocks.h>
#include <lib/unittest.h>
#include <platform/pc/hpet.h>

#if !X86_LEGACY

static bool within_percent(uint64_t a, uint64_t b, uint64_t percent) {
    const uint64_t diff = (a > b) ? (a - b) : (b - a);
    return diff * 100 <= b * percent;
}

static bool test_cpuid_tsc_vs_hpet(void) {
    BEGIN_TEST;

    const uint64_t cpu_hz = x86_cpu_tsc_hz();
    if (cpu_hz == 0 || !hpet_is_available()) {
        unittest_printf("skipped, need both a cpuid TSC frequency and an HPET\n");
        END_TEST;
    }

    const uint64_t hpet_hz = hpet_calibrate_tsc();
    ASSERT_NE(0u, hpet_hz, "HPET calibration failed");
    unittest_printf("cpuid %llu Hz, HPET %llu Hz\n", cpu_hz, hpet_hz);
    EXPECT_TRUE(within_percent(cpu_hz, hpet_hz, 1), "cpuid TSC frequency disagrees with HPET");

    END_TEST;
}

BEGIN_TEST_CASE(pc_timer_tests)
RUN_TEST(test_cpuid_tsc_vs_hpet);
END_TEST_CASE(pc_timer_tests)

#endif
