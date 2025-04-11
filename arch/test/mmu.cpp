/*
 * Copyright (c) 2020 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#if ARCH_HAS_MMU

#include <arch/mmu.h>

#include <lk/cpp.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lib/unittest.h>
#include <kernel/vm.h>

namespace {

bool create_user_aspace() {
    BEGIN_TEST;

    if (arch_mmu_supports_user_aspaces()) {
        arch_aspace_t as;
        status_t err = arch_mmu_init_aspace(&as, USER_ASPACE_BASE, USER_ASPACE_SIZE, 0);
        ASSERT_EQ(NO_ERROR, err, "init aspace");

        err = arch_mmu_destroy_aspace(&as);
        EXPECT_EQ(NO_ERROR, err, "destroy");
    } else {
        arch_aspace_t as;
        status_t err = arch_mmu_init_aspace(&as, USER_ASPACE_BASE, USER_ASPACE_SIZE, 0);
        ASSERT_EQ(ERR_NOT_SUPPORTED, err, "init aspace");
    }

    END_TEST;
}

bool map_user_pages() {
    BEGIN_TEST;

    if (arch_mmu_supports_user_aspaces()) {
        arch_aspace_t as;
        status_t err = arch_mmu_init_aspace(&as, USER_ASPACE_BASE, USER_ASPACE_SIZE, 0);
        ASSERT_EQ(NO_ERROR, err, "init aspace");

        auto aspace_cleanup = lk::make_auto_call([&]() { arch_mmu_destroy_aspace(&as); });

        // allocate a batch of pages
        struct list_node pages = LIST_INITIAL_VALUE(pages);
        size_t count = pmm_alloc_pages(4, &pages);
        ASSERT_EQ(4U, count, "alloc pages");
        ASSERT_EQ(4U, list_length(&pages), "page list");

        auto pages_cleanup = lk::make_auto_call([&]() { pmm_free(&pages); });

        // map the pages into the address space
        vaddr_t va = USER_ASPACE_BASE;
        vm_page_t *p;
        list_for_every_entry(&pages, p, vm_page_t, node) {
            err = arch_mmu_map(&as, va, vm_page_to_paddr(p), 1, ARCH_MMU_FLAG_PERM_USER);
            EXPECT_LE(NO_ERROR, err, "map page");
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
        aspace_cleanup.cancel();
        err = arch_mmu_destroy_aspace(&as);
        EXPECT_EQ(NO_ERROR, err, "destroy");

        // free the pages we allocated before
        pages_cleanup.cancel();
        size_t freed = pmm_free(&pages);
        ASSERT_EQ(count, freed, "free");
    }

    END_TEST;
}

bool map_region_query_result(vmm_aspace_t *aspace, uint arch_flags) {
    BEGIN_TEST;
    void *ptr = NULL;

    // create a region of an arbitrary page in kernel aspace
    EXPECT_EQ(NO_ERROR, vmm_alloc(aspace, "test region", PAGE_SIZE, &ptr, 0, /* vmm_flags */ 0, arch_flags), "map region");
    EXPECT_NONNULL(ptr, "not null");

    // query the page to see if it's realistic
    {
        paddr_t pa = 0;
        uint flags = ~arch_flags;
        EXPECT_EQ(NO_ERROR, arch_mmu_query(&aspace->arch_aspace, (vaddr_t)ptr, &pa, &flags), "arch_query");
        EXPECT_NE(0U, pa, "valid pa");
        EXPECT_EQ(arch_flags, flags, "query flags");
    }

    // free this region we made
    EXPECT_EQ(NO_ERROR, vmm_free_region(aspace, (vaddr_t)ptr), "free region");

    // query that the page is not there anymore
    {
        paddr_t pa = 0;
        uint flags = ~arch_flags;
        EXPECT_EQ(ERR_NOT_FOUND, arch_mmu_query(&aspace->arch_aspace, (vaddr_t)ptr, &pa, &flags), "arch_query");
    }

    END_TEST;
}

bool map_region_expect_failure(vmm_aspace_t *aspace, uint arch_flags, int expected_error) {
    BEGIN_TEST;
    void *ptr = NULL;

    // create a region of an arbitrary page in kernel aspace
    EXPECT_EQ(expected_error, vmm_alloc(aspace, "test region", PAGE_SIZE, &ptr, 0, /* vmm_flags */ 0, arch_flags), "map region");
    EXPECT_NULL(ptr, "null");

    END_TEST;
}

bool map_query_pages() {
    BEGIN_TEST;

    vmm_aspace_t *kaspace = vmm_get_kernel_aspace();
    ASSERT_NONNULL(kaspace, "kaspace");

    // try mapping pages in the kernel address space with various permissions and read them back via arch query
    EXPECT_TRUE(map_region_query_result(kaspace, 0), "0");
    EXPECT_TRUE(map_region_query_result(kaspace, ARCH_MMU_FLAG_PERM_RO), "1");
    if (arch_mmu_supports_nx_mappings()) {
        EXPECT_TRUE(map_region_query_result(kaspace, ARCH_MMU_FLAG_PERM_NO_EXECUTE), "2");
        EXPECT_TRUE(map_region_query_result(kaspace, ARCH_MMU_FLAG_PERM_RO | ARCH_MMU_FLAG_PERM_NO_EXECUTE), "3");
    } else {
        EXPECT_TRUE(map_region_expect_failure(kaspace, ARCH_MMU_FLAG_PERM_NO_EXECUTE, ERR_INVALID_ARGS), "2");
        EXPECT_TRUE(map_region_expect_failure(kaspace, ARCH_MMU_FLAG_PERM_RO | ARCH_MMU_FLAG_PERM_NO_EXECUTE, ERR_INVALID_ARGS), "3");
    }

    EXPECT_TRUE(map_region_query_result(kaspace, ARCH_MMU_FLAG_PERM_USER), "4");
    EXPECT_TRUE(map_region_query_result(kaspace, ARCH_MMU_FLAG_PERM_USER | ARCH_MMU_FLAG_PERM_RO), "5");
    if (arch_mmu_supports_nx_mappings()) {
        EXPECT_TRUE(map_region_query_result(kaspace, ARCH_MMU_FLAG_PERM_USER | ARCH_MMU_FLAG_PERM_NO_EXECUTE), "6");
        EXPECT_TRUE(map_region_query_result(kaspace, ARCH_MMU_FLAG_PERM_USER | ARCH_MMU_FLAG_PERM_RO | ARCH_MMU_FLAG_PERM_NO_EXECUTE), "7");
    } else {
        EXPECT_TRUE(map_region_expect_failure(kaspace, ARCH_MMU_FLAG_PERM_USER | ARCH_MMU_FLAG_PERM_NO_EXECUTE, ERR_INVALID_ARGS), "6");
        EXPECT_TRUE(map_region_expect_failure(kaspace, ARCH_MMU_FLAG_PERM_USER | ARCH_MMU_FLAG_PERM_RO | ARCH_MMU_FLAG_PERM_NO_EXECUTE, ERR_INVALID_ARGS), "7");
    }

    END_TEST;
}

bool context_switch() {
    BEGIN_TEST;

    // create a user space, map a page or two and access it
    // NOTE: this assumes that kernel code can directly access user space, which isn't necessarily true
    // on all architectures. See SMAP on x86, PAN on ARM, and SUM on RISC-V.
    if (arch_mmu_supports_user_aspaces()) {
        arch_aspace_t as;
        status_t err = arch_mmu_init_aspace(&as, USER_ASPACE_BASE, USER_ASPACE_SIZE, 0);
        ASSERT_EQ(NO_ERROR, err, "init aspace");
        auto aspace_cleanup = lk::make_auto_call([&]() { arch_mmu_destroy_aspace(&as); });

        // switch to the address space
        arch_mmu_context_switch(&as);
        auto cleanup_switch = lk::make_auto_call([&]() { arch_mmu_context_switch(NULL); });

        // map a page, verify can be read through the page
        vm_page_t *p = pmm_alloc_page();
        ASSERT_NONNULL(p, "page");
        auto page_cleanup = lk::make_auto_call([&]() { pmm_free_page(p); });

        // map it
        err = arch_mmu_map(&as, USER_ASPACE_BASE, vm_page_to_paddr(p), 1, ARCH_MMU_FLAG_PERM_USER);
        ASSERT_LE(NO_ERROR, err, "map");

        // write a known value to the kvaddr portion of the page
        volatile int *kv = static_cast<volatile int *>(paddr_to_kvaddr(vm_page_to_paddr(p)));
        *kv = 99;

        // read the data back from the page
        volatile int *ptr = reinterpret_cast<volatile int *>(USER_ASPACE_BASE);
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
        cleanup_switch.cancel();
        arch_mmu_context_switch(NULL);

        // destroy it
        aspace_cleanup.cancel();
        err = arch_mmu_destroy_aspace(&as);
        EXPECT_EQ(NO_ERROR, err, "destroy");

        // free the page
        page_cleanup.cancel();
        size_t c = pmm_free_page(p);
        EXPECT_EQ(1U, c, "free");
    }

    END_TEST;
}

BEGIN_TEST_CASE(arch_mmu_tests)
RUN_TEST(create_user_aspace);
RUN_TEST(map_user_pages);
RUN_TEST(map_query_pages);
RUN_TEST(context_switch);
END_TEST_CASE(arch_mmu_tests)

} // namespace

#endif // ARCH_HAS_MMU
