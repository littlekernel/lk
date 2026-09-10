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
#include <lktl/auto_call.h>
#include <lib/unittest.h>
#include <arch/ops.h>
#include <kernel/event.h>
#include <kernel/mp.h>
#include <kernel/thread.h>
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

        // unmap them again, which also frees the tables they needed
        err = arch_mmu_unmap(&as, USER_ASPACE_BASE, count);
        EXPECT_LE(NO_ERROR, err, "unmap");

        // destroy the now empty aspace
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

// The tests below read and write user addresses from kernel mode while a user
// aspace is active. That assumes the kernel may access user memory directly,
// which holds as long as nothing turns on SMAP (x86), PAN (arm64) or clears
// SUM (riscv).

// Fill a fresh page with a pattern and map it at va in the aspace.
vm_page_t *map_pattern_page(vmm_aspace_t *as, vaddr_t va, int pattern) {
    vm_page_t *p = pmm_alloc_page();
    if (!p) {
        return nullptr;
    }
    volatile int *kv = static_cast<volatile int *>(paddr_to_kvaddr(vm_page_to_paddr(p)));
    *kv = pattern;
    if (arch_mmu_map(&as->arch_aspace, va, vm_page_to_paddr(p), 1, ARCH_MMU_FLAG_PERM_USER) < 0) {
        pmm_free_page(p);
        return nullptr;
    }
    return p;
}

int read_user(vaddr_t va) {
    return *reinterpret_cast<volatile int *>(va);
}

// create a user aspace, map a page, make the aspace active and access the page through its low address
bool context_switch() {
    BEGIN_TEST;

    if (!arch_mmu_supports_user_aspaces()) {
        END_TEST;
    }

    vmm_aspace_t *as = nullptr;
    ASSERT_EQ(NO_ERROR, vmm_create_aspace(&as, "context_switch", 0), "create aspace");
    auto aspace_cleanup = lk::make_auto_call([&]() {
        vmm_set_active_aspace(nullptr);
        vmm_free_aspace(as);
    });

    vm_page_t *p = pmm_alloc_page();
    ASSERT_NONNULL(p, "page");
    auto page_cleanup = lk::make_auto_call([&]() { pmm_free_page(p); });

    int err = arch_mmu_map(&as->arch_aspace, USER_ASPACE_BASE, vm_page_to_paddr(p), 1, ARCH_MMU_FLAG_PERM_USER);
    ASSERT_LE(NO_ERROR, err, "map");

    // switch through the vmm so the scheduler knows which aspace this thread holds
    EXPECT_NULL(vmm_set_active_aspace(as), "no user aspace before");

    // write a known value to the kvaddr portion of the page and read it back through the low address
    volatile int *kv = static_cast<volatile int *>(paddr_to_kvaddr(vm_page_to_paddr(p)));
    *kv = 99;
    EXPECT_EQ(99, read_user(USER_ASPACE_BASE), "readback");
    *kv = 0xaa;
    EXPECT_EQ(0xaa, read_user(USER_ASPACE_BASE), "readback 2");

    // write to the page and read it back from the kernel side
    *reinterpret_cast<volatile int *>(USER_ASPACE_BASE) = 0x55;
    EXPECT_EQ(0x55, *kv, "readback 3");

    // switch back to the kernel aspace
    EXPECT_EQ(as, vmm_set_active_aspace(nullptr), "switch back");

    // unmap the page so the aspace is empty, then destroy it
    err = arch_mmu_unmap(&as->arch_aspace, USER_ASPACE_BASE, 1);
    EXPECT_LE(NO_ERROR, err, "unmap");
    aspace_cleanup.cancel();
    EXPECT_EQ(NO_ERROR, vmm_free_aspace(as), "free aspace");

    page_cleanup.cancel();
    EXPECT_EQ(1U, pmm_free_page(p), "free page");

    END_TEST;
}

// Unmap a page and map a different one at the same address while the aspace is
// active: a translation left in the TLB by the unmap would still read the old page.
bool unmap_user() {
    BEGIN_TEST;

    if (!arch_mmu_supports_user_aspaces()) {
        END_TEST;
    }

    vmm_aspace_t *as = nullptr;
    ASSERT_EQ(NO_ERROR, vmm_create_aspace(&as, "unmap_user", 0), "create aspace");
    auto aspace_cleanup = lk::make_auto_call([&]() {
        vmm_set_active_aspace(nullptr);
        arch_mmu_unmap(&as->arch_aspace, USER_ASPACE_BASE, 1);
        vmm_free_aspace(as);
    });

    vm_page_t *p1 = map_pattern_page(as, USER_ASPACE_BASE, 0x11);
    ASSERT_NONNULL(p1, "map first page");
    auto p1_cleanup = lk::make_auto_call([&]() { pmm_free_page(p1); });

    vmm_set_active_aspace(as);
    EXPECT_EQ(0x11, read_user(USER_ASPACE_BASE), "first page");

    EXPECT_LE(NO_ERROR, arch_mmu_unmap(&as->arch_aspace, USER_ASPACE_BASE, 1), "unmap");
    paddr_t pa;
    EXPECT_EQ(ERR_NOT_FOUND, arch_mmu_query(&as->arch_aspace, USER_ASPACE_BASE, &pa, nullptr), "gone");

    vm_page_t *p2 = map_pattern_page(as, USER_ASPACE_BASE, 0x22);
    ASSERT_NONNULL(p2, "map second page");
    auto p2_cleanup = lk::make_auto_call([&]() { pmm_free_page(p2); });
    EXPECT_EQ(0x22, read_user(USER_ASPACE_BASE), "second page, not a stale translation of the first");

    END_TEST;
}

// Two aspaces mapping the same address to different pages; switching between
// them must never show the other's page.
bool two_aspaces() {
    BEGIN_TEST;

    if (!arch_mmu_supports_user_aspaces()) {
        END_TEST;
    }

    vmm_aspace_t *a = nullptr;
    vmm_aspace_t *b = nullptr;
    ASSERT_EQ(NO_ERROR, vmm_create_aspace(&a, "two_aspaces_a", 0), "create a");
    auto a_cleanup = lk::make_auto_call([&]() {
        vmm_set_active_aspace(nullptr);
        arch_mmu_unmap(&a->arch_aspace, USER_ASPACE_BASE, 1);
        vmm_free_aspace(a);
    });
    ASSERT_EQ(NO_ERROR, vmm_create_aspace(&b, "two_aspaces_b", 0), "create b");
    auto b_cleanup = lk::make_auto_call([&]() {
        arch_mmu_unmap(&b->arch_aspace, USER_ASPACE_BASE, 1);
        vmm_free_aspace(b);
    });

    vm_page_t *pa = map_pattern_page(a, USER_ASPACE_BASE, 0xaa);
    ASSERT_NONNULL(pa, "map a");
    auto pa_cleanup = lk::make_auto_call([&]() { pmm_free_page(pa); });
    vm_page_t *pb = map_pattern_page(b, USER_ASPACE_BASE, 0xbb);
    ASSERT_NONNULL(pb, "map b");
    auto pb_cleanup = lk::make_auto_call([&]() { pmm_free_page(pb); });

    for (int i = 0; i < 4; i++) {
        vmm_set_active_aspace(a);
        EXPECT_EQ(0xaa, read_user(USER_ASPACE_BASE), "a's page");
        vmm_set_active_aspace(b);
        EXPECT_EQ(0xbb, read_user(USER_ASPACE_BASE), "b's page");
    }
    // via the kernel-only state in between as well
    vmm_set_active_aspace(nullptr);
    vmm_set_active_aspace(a);
    EXPECT_EQ(0xaa, read_user(USER_ASPACE_BASE), "a's page after kernel only");

    END_TEST;
}

// Create, use and destroy aspaces in a loop with alternating pages behind the
// same address. Where user aspaces share one asid this reuses it every time;
// with per aspace asids it exercises allocation and release.
bool aspace_churn() {
    BEGIN_TEST;

    if (!arch_mmu_supports_user_aspaces()) {
        END_TEST;
    }

    for (int i = 0; i < 32; i++) {
        vmm_aspace_t *as = nullptr;
        ASSERT_EQ(NO_ERROR, vmm_create_aspace(&as, "churn", 0), "create aspace");
        const int pattern = 0x1000 + i;
        vm_page_t *p = map_pattern_page(as, USER_ASPACE_BASE, pattern);
        ASSERT_NONNULL(p, "map");

        vmm_set_active_aspace(as);
        int seen = read_user(USER_ASPACE_BASE);
        vmm_set_active_aspace(nullptr);

        EXPECT_LE(NO_ERROR, arch_mmu_unmap(&as->arch_aspace, USER_ASPACE_BASE, 1), "unmap");
        EXPECT_EQ(NO_ERROR, vmm_free_aspace(as), "free aspace");
        pmm_free_page(p);

        if (seen != pattern) {
            EXPECT_EQ(pattern, seen, "page seen through a fresh aspace");
            break;
        }
    }

    END_TEST;
}

// ranges that leave the aspace are refused before anything is touched
bool out_of_range() {
    BEGIN_TEST;

    if (!arch_mmu_supports_user_aspaces()) {
        END_TEST;
    }

    vmm_aspace_t *as = nullptr;
    ASSERT_EQ(NO_ERROR, vmm_create_aspace(&as, "out_of_range", 0), "create aspace");
    auto aspace_cleanup = lk::make_auto_call([&]() { vmm_free_aspace(as); });

    vm_page_t *p = pmm_alloc_page();
    ASSERT_NONNULL(p, "page");
    auto page_cleanup = lk::make_auto_call([&]() { pmm_free_page(p); });
    const paddr_t pa = vm_page_to_paddr(p);
    arch_aspace_t *arch = &as->arch_aspace;

    // starts inside, runs past the top
    const vaddr_t last = USER_ASPACE_BASE + USER_ASPACE_SIZE - PAGE_SIZE;
    EXPECT_EQ(ERR_OUT_OF_RANGE, arch_mmu_map(arch, last, pa, 2, ARCH_MMU_FLAG_PERM_USER), "map past the top");
    EXPECT_EQ(ERR_OUT_OF_RANGE, arch_mmu_unmap(arch, last, 2), "unmap past the top");

    // entirely outside, in the kernel's half
    EXPECT_EQ(ERR_OUT_OF_RANGE, arch_mmu_map(arch, KERNEL_ASPACE_BASE, pa, 1, ARCH_MMU_FLAG_PERM_USER), "map kernel address");
    EXPECT_EQ(ERR_OUT_OF_RANGE, arch_mmu_unmap(arch, KERNEL_ASPACE_BASE, 1), "unmap kernel address");
    paddr_t qpa;
    EXPECT_EQ(ERR_OUT_OF_RANGE, arch_mmu_query(arch, KERNEL_ASPACE_BASE, &qpa, nullptr), "query kernel address");

    // just below the aspace, when there is room below it
    if (USER_ASPACE_BASE >= PAGE_SIZE) {
        EXPECT_EQ(ERR_OUT_OF_RANGE, arch_mmu_map(arch, USER_ASPACE_BASE - PAGE_SIZE, pa, 1, ARCH_MMU_FLAG_PERM_USER), "map below");
    }

    // the last page by itself is fine
    EXPECT_LE(NO_ERROR, arch_mmu_map(arch, last, pa, 1, ARCH_MMU_FLAG_PERM_USER), "map last page");
    EXPECT_EQ(NO_ERROR, arch_mmu_query(arch, last, &qpa, nullptr), "query last page");
    EXPECT_EQ(pa, qpa, "last page pa");
    EXPECT_LE(NO_ERROR, arch_mmu_unmap(arch, last, 1), "unmap last page");

    END_TEST;
}

// Pages far apart need their own intermediate tables; after unmapping them the
// aspace must be back to just its top level table, which destroy asserts.
bool table_reclaim() {
    BEGIN_TEST;

    if (!arch_mmu_supports_user_aspaces()) {
        END_TEST;
    }

    vmm_aspace_t *as = nullptr;
    ASSERT_EQ(NO_ERROR, vmm_create_aspace(&as, "table_reclaim", 0), "create aspace");
    auto aspace_cleanup = lk::make_auto_call([&]() { vmm_free_aspace(as); });

    // spread over the aspace so no two share a lower level table; the size
    // need not be a multiple of four pages, so round the middle ones down
    const vaddr_t vas[] = {
        USER_ASPACE_BASE,
        ROUNDDOWN(USER_ASPACE_BASE + USER_ASPACE_SIZE / 4, PAGE_SIZE),
        ROUNDDOWN(USER_ASPACE_BASE + USER_ASPACE_SIZE / 2, PAGE_SIZE),
        USER_ASPACE_BASE + USER_ASPACE_SIZE - PAGE_SIZE,
    };
    vm_page_t *pages[countof(vas)] = {};
    auto pages_cleanup = lk::make_auto_call([&]() {
        for (auto *p : pages) {
            if (p) pmm_free_page(p);
        }
    });

    for (size_t i = 0; i < countof(vas); i++) {
        pages[i] = map_pattern_page(as, vas[i], (int)i);
        ASSERT_NONNULL(pages[i], "map");
    }
    vmm_set_active_aspace(as);
    for (size_t i = 0; i < countof(vas); i++) {
        EXPECT_EQ((int)i, read_user(vas[i]), "read back");
    }
    vmm_set_active_aspace(nullptr);
    for (size_t i = 0; i < countof(vas); i++) {
        EXPECT_LE(NO_ERROR, arch_mmu_unmap(&as->arch_aspace, vas[i], 1), "unmap");
        paddr_t pa;
        EXPECT_EQ(ERR_NOT_FOUND, arch_mmu_query(&as->arch_aspace, vas[i], &pa, nullptr), "gone");
    }

    aspace_cleanup.cancel();
    EXPECT_EQ(NO_ERROR, vmm_free_aspace(as), "free aspace with only the top table left");

    END_TEST;
}

// A thread on another cpu keeps the aspace loaded and a translation of va
// cached while this cpu unmaps the page and maps another one at the same
// address. The unmap's shootdown must reach that cpu or it reads the old page.
struct shootdown_args {
    vmm_aspace_t *aspace;
    vaddr_t va;
    event_t loaded;
    volatile int phase;   // 0: hold off, 1: read again and finish
    volatile int first;
    volatile int second;
};

static int shootdown_thread(void *arg) {
    auto *args = static_cast<shootdown_args *>(arg);

    vmm_set_active_aspace(args->aspace);
    args->first = read_user(args->va);
    event_signal(&args->loaded, true);

    // keep the aspace loaded, and the translation cached, without touching va
    while (__atomic_load_n(&args->phase, __ATOMIC_ACQUIRE) == 0) {
        arch_spinloop_pause();
    }
    args->second = read_user(args->va);

    vmm_set_active_aspace(nullptr);
    return 0;
}

bool smp_shootdown() {
    BEGIN_TEST;

    if (!arch_mmu_supports_user_aspaces()) {
        END_TEST;
    }

    // stay off the cpu the other thread runs on, or landing there would switch it out
    thread_t *self = get_current_thread();
    thread_set_pinned_cpu(self, arch_curr_cpu_num());
    thread_yield();
    auto unpin = lk::make_auto_call([&]() { thread_set_pinned_cpu(self, -1); });
    const mp_cpu_mask_t others = mp_get_active_mask() & ~(1U << arch_curr_cpu_num());
    if (others == 0) {
        unittest_printf(" (needs a second cpu)");
        END_TEST;
    }
    const uint cpu = __builtin_ctz(others);

    shootdown_args args = {};
    ASSERT_EQ(NO_ERROR, vmm_create_aspace(&args.aspace, "shootdown", 0), "create aspace");
    auto aspace_cleanup = lk::make_auto_call([&]() {
        arch_mmu_unmap(&args.aspace->arch_aspace, USER_ASPACE_BASE, 1);
        vmm_free_aspace(args.aspace);
    });
    args.va = USER_ASPACE_BASE;
    event_init(&args.loaded, false, 0);

    vm_page_t *p1 = map_pattern_page(args.aspace, args.va, 0x11);
    ASSERT_NONNULL(p1, "map first page");
    auto p1_cleanup = lk::make_auto_call([&]() { pmm_free_page(p1); });

    thread_t *t = thread_create("shootdown", shootdown_thread, &args, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    ASSERT_NONNULL(t, "thread_create");
    thread_set_pinned_cpu(t, cpu);
    thread_resume(t);
    ASSERT_EQ(NO_ERROR, event_wait_timeout(&args.loaded, 5000), "other cpu loaded the aspace");
    EXPECT_EQ(0x11, args.first, "other cpu read the first page");

    // replace the page under it
    EXPECT_LE(NO_ERROR, arch_mmu_unmap(&args.aspace->arch_aspace, args.va, 1), "unmap");
    vm_page_t *p2 = map_pattern_page(args.aspace, args.va, 0x22);
    ASSERT_NONNULL(p2, "map second page");
    auto p2_cleanup = lk::make_auto_call([&]() { pmm_free_page(p2); });

    __atomic_store_n(&args.phase, 1, __ATOMIC_RELEASE);
    int retcode = -1;
    EXPECT_EQ(NO_ERROR, thread_join(t, &retcode, 5000), "join");
    EXPECT_EQ(0x22, args.second, "other cpu saw the new page, not its cached translation");

    END_TEST;
}

// Holds an aspace loaded on its (pinned) cpu until told to let go. It spins
// rather than blocks: a blocked thread is switched out, and the switch unloads
// the aspace from the cpu, which is exactly the state this test must avoid.
struct busy_aspace_args {
    vmm_aspace_t *aspace;
    event_t loaded;
    volatile int release;
};

static int busy_aspace_thread(void *arg) {
    auto *args = static_cast<busy_aspace_args *>(arg);

    vmm_set_active_aspace(args->aspace);
    event_signal(&args->loaded, true);
    while (!__atomic_load_n(&args->release, __ATOMIC_ACQUIRE)) {
        arch_spinloop_pause();
    }
    vmm_set_active_aspace(nullptr);
    return 0;
}

bool free_active_aspace() {
    BEGIN_TEST;

    if (!arch_mmu_supports_user_aspaces()) {
        END_TEST;
    }

    // freeing an aspace the current thread has loaded switches it off first and succeeds
    vmm_aspace_t *as = nullptr;
    ASSERT_EQ(NO_ERROR, vmm_create_aspace(&as, "free_active", 0), "create");
    vmm_set_active_aspace(as);
    EXPECT_EQ(NO_ERROR, vmm_free_aspace(as), "free while active on this cpu");
    EXPECT_NULL(get_current_thread()->aspace, "switched off");

    // with another cpu holding it, the free is refused until that cpu lets go.
    // Pin ourselves so we cannot land on the cpu holding it and unload it by
    // being scheduled there.
    thread_t *self = get_current_thread();
    thread_set_pinned_cpu(self, arch_curr_cpu_num());
    thread_yield();
    const uint my_cpu = arch_curr_cpu_num();
    const mp_cpu_mask_t others = mp_get_active_mask() & ~(1U << my_cpu);
    if (others != 0) {
        const uint cpu = __builtin_ctz(others);

        busy_aspace_args args = {};
        ASSERT_EQ(NO_ERROR, vmm_create_aspace(&args.aspace, "free_busy", 0), "create");
        event_init(&args.loaded, false, 0);

        thread_t *t = thread_create("busy_aspace", busy_aspace_thread, &args,
                                    DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
        ASSERT_NONNULL(t, "thread_create");
        thread_set_pinned_cpu(t, cpu);
        thread_resume(t);
        ASSERT_EQ(NO_ERROR, event_wait_timeout(&args.loaded, 5000), "aspace loaded on other cpu");

        // only an arch that counts loaded cpus can refuse; the others report 0
        if (arch_aspace_active_cpus(&args.aspace->arch_aspace) > 0) {
            EXPECT_EQ(ERR_BUSY, vmm_free_aspace(args.aspace), "free while loaded elsewhere");
        } else {
            unittest_printf(" (arch does not track loaded cpus)");
        }

        __atomic_store_n(&args.release, 1, __ATOMIC_RELEASE);
        int retcode = -1;
        EXPECT_EQ(NO_ERROR, thread_join(t, &retcode, 5000), "join");
        EXPECT_EQ(0, retcode, "thread return");

        EXPECT_EQ(NO_ERROR, vmm_free_aspace(args.aspace), "free after release");
    }
    thread_set_pinned_cpu(self, -1);

    END_TEST;
}

// page tables an aspace holds, root included; only riscv exposes the list
bool check_table_count(vmm_aspace_t *as, size_t expected) {
#if ARCH_RISCV
    return list_length(&as->arch_aspace.pt_list) == expected;
#else
    return true;
#endif
}

#if ARCH_RISCV
constexpr size_t pt_levels = RISCV_MMU_PT_LEVELS;
#else
constexpr size_t pt_levels = 0;
#endif

// the first address above the user base that is aligned to size
vaddr_t user_boundary(size_t size) {
    return ROUNDUP(USER_ASPACE_BASE + 1, size);
}

// A range mapped in one call that crosses table boundaries: after each page
// the walk resumes in the right table at every level, including where a
// boundary at every lower level falls at once.
bool map_across_table_boundaries() {
    BEGIN_TEST;

    if (!arch_mmu_supports_user_aspaces()) {
        END_TEST;
    }

    vmm_aspace_t *as = nullptr;
    ASSERT_EQ(NO_ERROR, vmm_create_aspace(&as, "boundaries", 0), "create aspace");
    auto aspace_cleanup = lk::make_auto_call([&]() { vmm_free_aspace(as); });

    struct list_node pages = LIST_INITIAL_VALUE(pages);
    paddr_t pa;
    ASSERT_EQ(4U, pmm_alloc_contiguous(4, PAGE_SIZE_SHIFT, &pa, &pages), "alloc pages");
    auto pages_cleanup = lk::make_auto_call([&]() { pmm_free(&pages); });

    // two pages either side of a 1GB boundary, then of a 2MB one. On riscv the
    // first needs a table per level below the 1GB one on each side, the
    // second two leaf tables under a shared chain.
    const struct {
        vaddr_t base;
        size_t tables;
    } ranges[] = {
        { user_boundary(1UL << 30) - 2 * PAGE_SIZE, pt_levels + 2 },
        { user_boundary(2UL << 20) - 2 * PAGE_SIZE, pt_levels + 1 },
    };
    for (const auto &r : ranges) {
        ASSERT_LE(NO_ERROR, arch_mmu_map(&as->arch_aspace, r.base, pa, 4, ARCH_MMU_FLAG_PERM_USER), "map");
        EXPECT_TRUE(check_table_count(as, r.tables), "tables after map");
        for (uint i = 0; i < 4; i++) {
            paddr_t got;
            uint flags;
            EXPECT_EQ(NO_ERROR, arch_mmu_query(&as->arch_aspace, r.base + i * PAGE_SIZE, &got, &flags), "query");
            EXPECT_EQ(pa + i * PAGE_SIZE, got, "paddr");
            EXPECT_EQ(ARCH_MMU_FLAG_PERM_USER, flags, "flags");
        }
        EXPECT_LE(NO_ERROR, arch_mmu_unmap(&as->arch_aspace, r.base, 4), "unmap");
        for (uint i = 0; i < 4; i++) {
            paddr_t got;
            EXPECT_EQ(ERR_NOT_FOUND, arch_mmu_query(&as->arch_aspace, r.base + i * PAGE_SIZE, &got, nullptr), "gone");
        }
        EXPECT_TRUE(check_table_count(as, 1), "tables after unmap");
    }

    aspace_cleanup.cancel();
    EXPECT_EQ(NO_ERROR, vmm_free_aspace(as), "free aspace");

    END_TEST;
}

// Unmapping steps over holes without losing the pages after them, whether
// the hole is a few entries or a whole missing table, and a range with
// nothing in it is not an error.
bool unmap_range_with_holes() {
    BEGIN_TEST;

    if (!arch_mmu_supports_user_aspaces()) {
        END_TEST;
    }

    vmm_aspace_t *as = nullptr;
    ASSERT_EQ(NO_ERROR, vmm_create_aspace(&as, "holes", 0), "create aspace");
    auto aspace_cleanup = lk::make_auto_call([&]() { vmm_free_aspace(as); });

    const vaddr_t base = USER_ASPACE_BASE;
    vm_page_t *p2 = map_pattern_page(as, base + 2 * PAGE_SIZE, 2);
    ASSERT_NONNULL(p2, "map page 2");
    auto p2_cleanup = lk::make_auto_call([&]() { pmm_free_page(p2); });
    vm_page_t *p5 = map_pattern_page(as, base + 5 * PAGE_SIZE, 5);
    ASSERT_NONNULL(p5, "map page 5");
    auto p5_cleanup = lk::make_auto_call([&]() { pmm_free_page(p5); });

    EXPECT_LE(NO_ERROR, arch_mmu_unmap(&as->arch_aspace, base, 8), "unmap around holes");
    paddr_t pa;
    EXPECT_EQ(ERR_NOT_FOUND, arch_mmu_query(&as->arch_aspace, base + 2 * PAGE_SIZE, &pa, nullptr), "page 2 gone");
    EXPECT_EQ(ERR_NOT_FOUND, arch_mmu_query(&as->arch_aspace, base + 5 * PAGE_SIZE, &pa, nullptr), "page 5 gone");
    EXPECT_TRUE(check_table_count(as, 1), "tables reclaimed");
    EXPECT_LE(NO_ERROR, arch_mmu_unmap(&as->arch_aspace, base, 8), "unmap nothing");

    // a page in the leaf table after a missing one: an unmap starting in the
    // hole must stop at the end of the hole, not the end of the range
    const vaddr_t beyond = user_boundary(2UL << 20);
    vm_page_t *pb = map_pattern_page(as, beyond, 7);
    ASSERT_NONNULL(pb, "map page beyond the hole");
    auto pb_cleanup = lk::make_auto_call([&]() { pmm_free_page(pb); });
    const vaddr_t hole = beyond - (2UL << 20) >= USER_ASPACE_BASE ? beyond - (2UL << 20) : USER_ASPACE_BASE;
    const uint span = (beyond - hole) / PAGE_SIZE + 512;
    EXPECT_LE(NO_ERROR, arch_mmu_unmap(&as->arch_aspace, hole, span), "unmap from inside a hole");
    EXPECT_EQ(ERR_NOT_FOUND, arch_mmu_query(&as->arch_aspace, beyond, &pa, nullptr), "page beyond gone");
    EXPECT_TRUE(check_table_count(as, 1), "tables reclaimed again");

    aspace_cleanup.cancel();
    EXPECT_EQ(NO_ERROR, vmm_free_aspace(as), "free aspace");

    END_TEST;
}

// A range filling one leaf table and parts of two more, on both sides of a
// 1GB boundary, goes in and comes out in one call each, and the unmap takes
// every table it emptied with it, at every level.
bool reclaim_after_big_range() {
    BEGIN_TEST;

    if (!arch_mmu_supports_user_aspaces()) {
        END_TEST;
    }

    vmm_aspace_t *as = nullptr;
    ASSERT_EQ(NO_ERROR, vmm_create_aspace(&as, "big_range", 0), "create aspace");
    auto aspace_cleanup = lk::make_auto_call([&]() { vmm_free_aspace(as); });

    constexpr uint count = 1024;
    struct list_node pages = LIST_INITIAL_VALUE(pages);
    paddr_t pa;
    ASSERT_EQ((size_t)count, pmm_alloc_contiguous(count, PAGE_SIZE_SHIFT, &pa, &pages), "alloc pages");
    auto pages_cleanup = lk::make_auto_call([&]() { pmm_free(&pages); });

    // 256 pages before the boundary, 768 after
    const vaddr_t base = user_boundary(1UL << 30) - 256 * PAGE_SIZE;
    ASSERT_LE(NO_ERROR, arch_mmu_map(&as->arch_aspace, base, pa, count, ARCH_MMU_FLAG_PERM_USER), "map");
    // riscv: a chain down to the 1GB level, a 2MB level table on each side, three leaf tables
    EXPECT_TRUE(check_table_count(as, pt_levels + 3), "tables after map");

    for (uint i = 0; i < count; i += 255) {
        paddr_t got;
        EXPECT_EQ(NO_ERROR, arch_mmu_query(&as->arch_aspace, base + i * PAGE_SIZE, &got, nullptr), "query");
        EXPECT_EQ(pa + i * PAGE_SIZE, got, "paddr");
    }

    EXPECT_LE(NO_ERROR, arch_mmu_unmap(&as->arch_aspace, base, count), "unmap");
    for (uint i = 0; i < count; i += 255) {
        paddr_t got;
        EXPECT_EQ(ERR_NOT_FOUND, arch_mmu_query(&as->arch_aspace, base + i * PAGE_SIZE, &got, nullptr), "gone");
    }
    EXPECT_TRUE(check_table_count(as, 1), "tables after unmap");

    aspace_cleanup.cancel();
    EXPECT_EQ(NO_ERROR, vmm_free_aspace(as), "free aspace");

    END_TEST;
}

// A 2MB aligned range with 2MB aligned backing maps as one large page: a
// query inside it resolves, part of it cannot be unmapped on its own where
// the arch keeps it whole, and the whole of it can.
bool large_page_map() {
    BEGIN_TEST;

    if (!arch_mmu_supports_user_aspaces()) {
        END_TEST;
    }

    vmm_aspace_t *as = nullptr;
    ASSERT_EQ(NO_ERROR, vmm_create_aspace(&as, "large_page", 0), "create aspace");
    auto aspace_cleanup = lk::make_auto_call([&]() { vmm_free_aspace(as); });

    constexpr size_t large = 2UL << 20;
    constexpr uint count = large / PAGE_SIZE;
    struct list_node pages = LIST_INITIAL_VALUE(pages);
    paddr_t pa;
    ASSERT_EQ((size_t)count, pmm_alloc_contiguous(count, 21, &pa, &pages), "alloc 2MB aligned");
    auto pages_cleanup = lk::make_auto_call([&]() { pmm_free(&pages); });

    const vaddr_t va = user_boundary(large);
    ASSERT_LE(NO_ERROR, arch_mmu_map(&as->arch_aspace, va, pa, count, ARCH_MMU_FLAG_PERM_USER), "map");
    // riscv: the chain down to the 2MB level and no leaf table
    EXPECT_TRUE(check_table_count(as, pt_levels - 1), "no leaf table");

    paddr_t got;
    uint flags;
    EXPECT_EQ(NO_ERROR, arch_mmu_query(&as->arch_aspace, va + 0x1234, &got, &flags), "query inside");
    EXPECT_EQ(pa + 0x1234, got, "paddr inside");
    EXPECT_EQ(ARCH_MMU_FLAG_PERM_USER, flags, "flags");

    int err = arch_mmu_unmap(&as->arch_aspace, va + PAGE_SIZE, 1);
    if (err == ERR_NOT_SUPPORTED) {
        EXPECT_EQ(NO_ERROR, arch_mmu_query(&as->arch_aspace, va + PAGE_SIZE, &got, nullptr), "still mapped");
    } else {
        EXPECT_LE(NO_ERROR, err, "partial unmap");
        EXPECT_EQ(ERR_NOT_FOUND, arch_mmu_query(&as->arch_aspace, va + PAGE_SIZE, &got, nullptr), "page split out");
    }

    EXPECT_LE(NO_ERROR, arch_mmu_unmap(&as->arch_aspace, va, count), "unmap whole");
    EXPECT_EQ(ERR_NOT_FOUND, arch_mmu_query(&as->arch_aspace, va, &got, nullptr), "gone");
    EXPECT_TRUE(check_table_count(as, 1), "tables reclaimed");

    aspace_cleanup.cancel();
    EXPECT_EQ(NO_ERROR, vmm_free_aspace(as), "free aspace");

    END_TEST;
}

#if ARCH_RISCV && LK_DEBUGLEVEL > 0
// When a page table cannot be allocated part way through, the pages already
// mapped are rolled back and the tables linked for the failing page are
// unlinked again, leaving the aspace as it was and still usable.
bool map_enomem_rollback() {
    BEGIN_TEST;

    if (!arch_mmu_supports_user_aspaces()) {
        END_TEST;
    }

    vmm_aspace_t *as = nullptr;
    ASSERT_EQ(NO_ERROR, vmm_create_aspace(&as, "enomem", 0), "create aspace");
    auto aspace_cleanup = lk::make_auto_call([&]() { vmm_free_aspace(as); });
    auto budget_cleanup = lk::make_auto_call([]() { riscv_mmu_set_ptable_alloc_budget(-1); });

    struct list_node pages = LIST_INITIAL_VALUE(pages);
    paddr_t pa;
    ASSERT_EQ(2U, pmm_alloc_contiguous(2, PAGE_SIZE_SHIFT, &pa, &pages), "alloc pages");
    auto pages_cleanup = lk::make_auto_call([&]() { pmm_free(&pages); });

    // two pages either side of a leaf table boundary
    const vaddr_t base = user_boundary(2UL << 20) - PAGE_SIZE;
    arch_aspace_t *aspace = &as->arch_aspace;
    paddr_t got;

    // no table at all
    riscv_mmu_set_ptable_alloc_budget(0);
    EXPECT_EQ(ERR_NO_MEMORY, arch_mmu_map(aspace, base, pa, 1, ARCH_MMU_FLAG_PERM_USER), "map with no tables");
    EXPECT_TRUE(check_table_count(as, 1), "nothing left behind");

    // the first table links in, the one below it does not: the empty one must go
    riscv_mmu_set_ptable_alloc_budget(1);
    EXPECT_EQ(ERR_NO_MEMORY, arch_mmu_map(aspace, base, pa, 1, ARCH_MMU_FLAG_PERM_USER), "map with one table");
    EXPECT_TRUE(check_table_count(as, 1), "empty table unlinked");

    // enough for the first page, not for the second page's leaf table
    riscv_mmu_set_ptable_alloc_budget(pt_levels - 1);
    EXPECT_EQ(ERR_NO_MEMORY, arch_mmu_map(aspace, base, pa, 2, ARCH_MMU_FLAG_PERM_USER), "map two pages");
    EXPECT_EQ(ERR_NOT_FOUND, arch_mmu_query(aspace, base, &got, nullptr), "first page rolled back");
    EXPECT_TRUE(check_table_count(as, 1), "first page's tables reclaimed");

    // and with the cap lifted the same mapping works
    riscv_mmu_set_ptable_alloc_budget(-1);
    EXPECT_LE(NO_ERROR, arch_mmu_map(aspace, base, pa, 2, ARCH_MMU_FLAG_PERM_USER), "map after rollback");
    EXPECT_EQ(NO_ERROR, arch_mmu_query(aspace, base + PAGE_SIZE, &got, nullptr), "query second page");
    EXPECT_EQ(pa + PAGE_SIZE, got, "paddr");
    EXPECT_LE(NO_ERROR, arch_mmu_unmap(aspace, base, 2), "unmap");
    EXPECT_TRUE(check_table_count(as, 1), "tables reclaimed");

    aspace_cleanup.cancel();
    EXPECT_EQ(NO_ERROR, vmm_free_aspace(as), "free aspace");

    END_TEST;
}
#endif

// The kernel aspace runs to the top of the address space, so its last page
// is the one place a range's end wraps to zero. The vmm never hands it out;
// map it directly, unless the platform's initial mappings already reach it.
bool kernel_aspace_top_page() {
    BEGIN_TEST;

    vmm_aspace_t *kas = vmm_get_kernel_aspace();
    vm_page_t *p = pmm_alloc_page();
    ASSERT_NONNULL(p, "alloc page");
    auto page_cleanup = lk::make_auto_call([&]() { pmm_free_page(p); });

    const vaddr_t va = KERNEL_ASPACE_BASE + KERNEL_ASPACE_SIZE - PAGE_SIZE;
    paddr_t pa;
    if (arch_mmu_query(&kas->arch_aspace, va, &pa, nullptr) == NO_ERROR) {
        unittest_printf(" (top page already mapped)");
        END_TEST;
    }
    ASSERT_LE(NO_ERROR, arch_mmu_map(&kas->arch_aspace, va, vm_page_to_paddr(p), 1, 0), "map top page");

    EXPECT_EQ(NO_ERROR, arch_mmu_query(&kas->arch_aspace, va, &pa, nullptr), "query");
    EXPECT_EQ(vm_page_to_paddr(p), pa, "paddr");
    EXPECT_LE(NO_ERROR, arch_mmu_unmap(&kas->arch_aspace, va, 1), "unmap");
    EXPECT_EQ(ERR_NOT_FOUND, arch_mmu_query(&kas->arch_aspace, va, &pa, nullptr), "gone");

    END_TEST;
}

BEGIN_TEST_CASE(arch_mmu_tests)
RUN_TEST(create_user_aspace);
RUN_TEST(map_user_pages);
RUN_TEST(map_query_pages);
RUN_TEST(context_switch);
RUN_TEST(unmap_user);
RUN_TEST(two_aspaces);
RUN_TEST(aspace_churn);
RUN_TEST(out_of_range);
RUN_TEST(table_reclaim);
RUN_TEST(map_across_table_boundaries);
RUN_TEST(unmap_range_with_holes);
RUN_TEST(reclaim_after_big_range);
RUN_TEST(large_page_map);
#if ARCH_RISCV && LK_DEBUGLEVEL > 0
RUN_TEST(map_enomem_rollback);
#endif
RUN_TEST(kernel_aspace_top_page);
RUN_TEST(free_active_aspace);
RUN_TEST(smp_shootdown);
END_TEST_CASE(arch_mmu_tests)

} // namespace

#endif // ARCH_HAS_MMU
