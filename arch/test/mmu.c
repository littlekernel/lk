/*
 * Copyright (c) 2020 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#if ARCH_HAS_MMU

#include <arch/mmu.h>

#include <lk/err.h>
#include <lib/unittest.h>
#include <kernel/vm.h>

static bool create_user_aspace(void) {
    BEGIN_TEST;

    arch_aspace_t as;
    status_t err = arch_mmu_init_aspace(&as, USER_ASPACE_BASE, USER_ASPACE_SIZE, 0);
    EXPECT_EQ(NO_ERROR, err, "init");

    err = arch_mmu_destroy_aspace(&as);
    EXPECT_EQ(NO_ERROR, err, "destroy");

    END_TEST;
}

BEGIN_TEST_CASE(arch_mmu_tests)
RUN_TEST(create_user_aspace);
END_TEST_CASE(arch_mmu_tests)

#endif // ARCH_HAS_MMU
