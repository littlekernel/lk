/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

// What the arch mmu tests share: arch calls made under the aspace lock, page
// table counts, physical pages to map, and a user aspace that cleans up after
// a test however it ends.

#include <arch/mmu.h>
#include <kernel/thread.h>
#include <kernel/vm.h>
#include <lk/cpp.h>
#include <lk/err.h>
#include <lk/list.h>
#include <sys/types.h>

namespace mmu_test {

constexpr uint kR = ARCH_MMU_FLAG_PERM_READ;
constexpr uint kW = ARCH_MMU_FLAG_PERM_WRITE;
constexpr uint kX = ARCH_MMU_FLAG_PERM_EXECUTE;
constexpr uint kRW = kR | kW;
constexpr uint kUserRW = ARCH_MMU_FLAG_PERM_USER | kRW;

// what a query reports for a page mapped with these flags: without NX every
// page can be executed
inline uint query_flags(uint flags) {
    return arch_mmu_supports_nx_mappings() ? flags : flags | kX;
}

// The arch calls with the aspace lock held, which is what serializes them
// against everything else that maps into the aspace. Anything a test does to
// the kernel aspace goes through these.
class aspace_lock_guard {
public:
    explicit aspace_lock_guard(vmm_aspace_t *as) : as_(as) { vmm_aspace_lock(as_); }
    ~aspace_lock_guard() { vmm_aspace_unlock(as_); }
    DISALLOW_COPY_ASSIGN_AND_MOVE(aspace_lock_guard);

private:
    vmm_aspace_t *as_;
};

inline status_t map(vmm_aspace_t *as, vaddr_t va, paddr_t pa, uint count, uint flags) {
    aspace_lock_guard guard(as);
    return arch_mmu_map(&as->arch_aspace, va, pa, count, flags);
}

inline status_t unmap(vmm_aspace_t *as, vaddr_t va, uint count) {
    aspace_lock_guard guard(as);
    return arch_mmu_unmap(&as->arch_aspace, va, count);
}

inline status_t protect(vmm_aspace_t *as, vaddr_t va, uint count, uint flags) {
    aspace_lock_guard guard(as);
    return arch_mmu_protect(&as->arch_aspace, va, count, flags);
}

inline status_t query(vmm_aspace_t *as, vaddr_t va, paddr_t *pa, uint *flags) {
    aspace_lock_guard guard(as);
    return arch_mmu_query(&as->arch_aspace, va, pa, flags);
}

// Counts the page tables this thread allocates and frees while it is in
// scope, and can make the allocations fail. One at a time. A release build
// has no hook to count with: everything reads as zero and no budget applies.
class table_tracker {
public:
#if LK_DEBUGLEVEL > 0
    table_tracker() { vm_ptable_track(true); }
    ~table_tracker() { vm_ptable_track(false); }

    size_t allocs() const {
        size_t a, f;
        vm_ptable_counts(&a, &f);
        return a;
    }
    size_t frees() const {
        size_t a, f;
        vm_ptable_counts(&a, &f);
        return f;
    }
    // fail the allocations after the next count, or never with -1
    void set_budget(int count) { vm_ptable_budget_set(count); }
#else
    table_tracker() = default;
    size_t allocs() const { return 0; }
    size_t frees() const { return 0; }
    void set_budget(int) {}
#endif
    DISALLOW_COPY_ASSIGN_AND_MOVE(table_tracker);

    // tables allocated and not freed since it came into scope
    long live() const { return (long)allocs() - (long)frees(); }
};

// physical pages for a test, contiguous and aligned as asked
struct backing {
    struct list_node pages = LIST_INITIAL_VALUE(pages);
    paddr_t pa = 0;
    size_t count = 0;

    bool alloc(size_t n, uint8_t align_log2 = PAGE_SIZE_SHIFT) {
        count = pmm_alloc_contiguous(n, align_log2, &pa, &pages);
        return count == n;
    }
    ~backing() { pmm_free(&pages); }
};

// Unmap everything in a user aspace. The count of an arch call cannot span a
// 64 bit aspace, so it goes a chunk at a time; a chunk boundary is a multiple
// of every large page size, so no chunk cuts one.
inline void unmap_all(vmm_aspace_t *as) {
    const vaddr_t last = as->base + (as->size - 1);
    vaddr_t va = as->base;
#if IS_64BIT
    const vaddr_t chunk = (vaddr_t)PAGE_SIZE << 30;
#else
    const vaddr_t chunk = 0; // the whole of it in one call
#endif
    for (;;) {
        const vaddr_t chunk_last = chunk ? MIN(ROUNDDOWN(va, chunk) + (chunk - 1), last) : last;
        unmap(as, va, (uint)((chunk_last - va) / PAGE_SIZE + 1));
        if (chunk_last == last) {
            break;
        }
        va = chunk_last + 1;
    }
}

// A user aspace for a test. Destroying an aspace that still maps something is
// an assert in the arch, not a test failure, so on the way out of a test that
// bailed this takes the aspace off the thread, unmaps whatever is left and
// frees it. A test that got to the end calls free() itself: that unmaps
// nothing, so a mapping or table the test left behind still trips the assert.
class scoped_user_aspace {
public:
    scoped_user_aspace() = default;
    ~scoped_user_aspace() {
        if (as_) {
            unload();
            unmap_all(as_);
            vmm_free_aspace(as_);
        }
    }
    DISALLOW_COPY_ASSIGN_AND_MOVE(scoped_user_aspace);

    status_t create(const char *name) { return vmm_create_aspace(&as_, name, 0); }

    status_t free() {
        unload();
        vmm_aspace_t *as = as_;
        as_ = nullptr;
        return vmm_free_aspace(as);
    }

    vmm_aspace_t *get() const { return as_; }
    vmm_aspace_t *operator->() const { return as_; }
    operator vmm_aspace_t *() const { return as_; }

private:
    void unload() {
        if (get_current_thread()->aspace == as_) {
            vmm_set_active_aspace(nullptr);
        }
    }

    vmm_aspace_t *as_ = nullptr;
};

} // namespace mmu_test
