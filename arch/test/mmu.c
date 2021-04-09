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
    ASSERT_EQ(NO_ERROR, err, "init");

    err = arch_mmu_destroy_aspace(&as);
    EXPECT_EQ(NO_ERROR, err, "destroy");

    END_TEST;
}

static bool map_user_pages(void) {
    BEGIN_TEST;

    arch_aspace_t as;
    status_t err = arch_mmu_init_aspace(&as, USER_ASPACE_BASE, USER_ASPACE_SIZE, 0);
    ASSERT_EQ(NO_ERROR, err, "init");

    // allocate a batch of pages
    struct list_node pages = LIST_INITIAL_VALUE(pages);
    size_t count = pmm_alloc_pages(4, &pages);
    EXPECT_EQ(4, count, "alloc pages");
    EXPECT_EQ(4, list_length(&pages), "page list");

    // map the pages into the address space
    vaddr_t va = USER_ASPACE_BASE;
    vm_page_t *p;
    list_for_every_entry(&pages, p, vm_page_t, node) {
        err = arch_mmu_map(&as, va, vm_page_to_paddr(p), 1, ARCH_MMU_FLAG_PERM_USER);
        EXPECT_EQ(NO_ERROR, err, "map page");
        va += PAGE_SIZE;
    }

    // query the pages to make sure they match
    va = USER_ASPACE_BASE;
    list_for_every_entry(&pages, p, vm_page_t, node) {
        paddr_t pa;
        uint flags;
        err = arch_mmu_query(&as, va, &pa, &flags);
        EXPECT_EQ(NO_ERROR, err, "query");
        EXPECT_EQ(vm_page_to_paddr(p), pa, "pa");
        EXPECT_EQ(ARCH_MMU_FLAG_PERM_USER, flags, "flags");
        va += PAGE_SIZE;

        //unittest_printf("\npa %#lx, flags %#x", pa, flags);
    }

    // destroy the aspace with the pages mapped
    err = arch_mmu_destroy_aspace(&as);
    EXPECT_EQ(NO_ERROR, err, "destroy");

    size_t freed = pmm_free(&pages);
    EXPECT_EQ(count, freed, "free");

    END_TEST;
}

static bool context_switch(void) {
    BEGIN_TEST;

    arch_aspace_t as;
    status_t err = arch_mmu_init_aspace(&as, USER_ASPACE_BASE, USER_ASPACE_SIZE, 0);
    ASSERT_EQ(NO_ERROR, err, "init");

    // switch to the address space
    arch_mmu_context_switch(&as);

    // map a page, verify can be read through the page
    vm_page_t *p = pmm_alloc_page();
    EXPECT_NONNULL(p, "page");

    // map it
    err = arch_mmu_map(&as, USER_ASPACE_BASE, vm_page_to_paddr(p), 1, ARCH_MMU_FLAG_PERM_USER);
    EXPECT_EQ(NO_ERROR, err, "map");

    // write a known value to the kvaddr portion of the page
    volatile int *kv = paddr_to_kvaddr(vm_page_to_paddr(p));
    *kv = 99;

    // read the data back from the page
    volatile int *ptr = (void *)USER_ASPACE_BASE;
    volatile int foo = *ptr;

    EXPECT_EQ(99, foo, "readback");
    *kv = 0xaa;
    foo = *ptr;
    EXPECT_EQ(0xaa, foo, "readback 2");

    // write to the page and read it back from the kernel side
    *ptr = 0x55;
    foo = *kv;
    EXPECT_EQ(0x55, foo, "readback 3");

    // switch back to kernel aspace
    arch_mmu_context_switch(NULL);

    // destroy it
    err = arch_mmu_destroy_aspace(&as);
    EXPECT_EQ(NO_ERROR, err, "destroy");

    // free the page
    size_t c = pmm_free_page(p);
    EXPECT_EQ(1, c, "free");

    END_TEST;
}

BEGIN_TEST_CASE(arch_mmu_tests)
RUN_TEST(create_user_aspace);
RUN_TEST(map_user_pages);
RUN_TEST(context_switch);
END_TEST_CASE(arch_mmu_tests)

#endif // ARCH_HAS_MMU
