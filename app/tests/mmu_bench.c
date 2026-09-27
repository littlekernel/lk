/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include "tests.h"

#if WITH_KERNEL_VM

#include <arch/mmu.h>
#include <arch/ops.h>
#include <inttypes.h>
#include <kernel/vm.h>
#include <lk/err.h>
#include <lk/list.h>
#include <platform.h>
#include <stdio.h>

// Times the arch mmu entry points on a private user aspace, in one large
// range and one page at a time (the vmm_alloc shape). Backing memory comes
// from the pmm so the map calls take a single physical base; it is 2MB
// aligned and the 4K rows map it one page off so nothing lines up for a
// large page, while the large page row maps it in place.
#define BENCH_PAGES 4096
#define BENCH_ALIGN_SHIFT 21
#define BENCH_ROUNDS 3

struct sample {
    lk_bigtime_t us;
    ulong cycles;
};

static void sample_start(struct sample *s) {
    s->us = current_time_hires();
    s->cycles = arch_cycle_count();
}

static void sample_stop(struct sample *s) {
    s->cycles = arch_cycle_count() - s->cycles;
    s->us = current_time_hires() - s->us;
}

static void keep_best(struct sample *best, const struct sample *s) {
    if (s->us < best->us) *best = *s;
}

static void report(const char *what, const struct sample *s) {
    printf("%-30s %8" PRIu64 " us %12lu cycles %6" PRIu64 " ns/page\n",
           what, s->us, s->cycles, s->us * 1000 / BENCH_PAGES);
}

static size_t table_count(vmm_aspace_t *as) {
#if ARCH_RISCV
    return list_length(&as->arch_aspace.pt_list);
#else
    return 0;
#endif
}

int mmu_bench(int argc, const console_cmd_args *argv) {
    vmm_aspace_t *as = NULL;
    status_t err = vmm_create_aspace(&as, "mmu_bench", 0);
    if (err < 0) {
        printf("failed to create aspace: %d\n", err);
        return err;
    }

    struct list_node pages = LIST_INITIAL_VALUE(pages);
    paddr_t pa;
    size_t got = pmm_alloc_contiguous(BENCH_PAGES, BENCH_ALIGN_SHIFT, &pa, &pages);
    if (got != BENCH_PAGES) {
        printf("failed to allocate %u contiguous pages\n", BENCH_PAGES);
        pmm_free(&pages);
        vmm_free_aspace(as);
        return ERR_NO_MEMORY;
    }

    arch_aspace_t *aspace = &as->arch_aspace;
    const vaddr_t va_large = ROUNDUP(USER_ASPACE_BASE + 1, 1UL << BENCH_ALIGN_SHIFT);
    const vaddr_t va = va_large + PAGE_SIZE;
    const uint flags = ARCH_MMU_FLAG_PERM_USER;
    struct sample best_map = { .us = ~0ULL }, best_query = best_map, best_unmap = best_map;
    struct sample best_map1 = best_map, best_unmap1 = best_map;
    struct sample best_map_large = best_map, best_unmap_large = best_map;
    size_t tables = 0, tables_large = 0;
    int ret = NO_ERROR;

    printf("mmu_bench: %u pages, best of %u rounds\n", BENCH_PAGES, BENCH_ROUNDS);
    for (uint round = 0; round < BENCH_ROUNDS && ret >= 0; round++) {
        struct sample s;

        // one call covering the whole range
        sample_start(&s);
        ret = arch_mmu_map(aspace, va, pa, BENCH_PAGES, flags);
        sample_stop(&s);
        if (ret < 0) {
            printf("map failed: %d\n", ret);
            break;
        }
        keep_best(&best_map, &s);
        tables = table_count(as);

        sample_start(&s);
        for (uint i = 0; i < BENCH_PAGES; i++) {
            paddr_t p;
            uint f;
            ret = arch_mmu_query(aspace, va + (vaddr_t)i * PAGE_SIZE, &p, &f);
            if (ret < 0 || p != pa + (paddr_t)i * PAGE_SIZE) {
                printf("query of page %u failed: %d\n", i, ret);
                ret = ERR_GENERIC;
                break;
            }
        }
        sample_stop(&s);
        if (ret < 0) break;
        keep_best(&best_query, &s);

        sample_start(&s);
        ret = arch_mmu_unmap(aspace, va, BENCH_PAGES);
        sample_stop(&s);
        if (ret < 0) {
            printf("unmap failed: %d\n", ret);
            break;
        }
        keep_best(&best_unmap, &s);

        // one page per call
        sample_start(&s);
        for (uint i = 0; i < BENCH_PAGES; i++) {
            ret = arch_mmu_map(aspace, va + (vaddr_t)i * PAGE_SIZE, pa + (paddr_t)i * PAGE_SIZE, 1, flags);
            if (ret < 0) break;
        }
        sample_stop(&s);
        if (ret < 0) {
            printf("single page map failed: %d\n", ret);
            arch_mmu_unmap(aspace, va, BENCH_PAGES);
            break;
        }
        keep_best(&best_map1, &s);

        sample_start(&s);
        for (uint i = 0; i < BENCH_PAGES; i++) {
            ret = arch_mmu_unmap(aspace, va + (vaddr_t)i * PAGE_SIZE, 1);
            if (ret < 0) break;
        }
        sample_stop(&s);
        if (ret < 0) {
            printf("single page unmap failed: %d\n", ret);
            arch_mmu_unmap(aspace, va, BENCH_PAGES);
            break;
        }
        keep_best(&best_unmap1, &s);

        // aligned, so large pages where the arch makes them
        sample_start(&s);
        ret = arch_mmu_map(aspace, va_large, pa, BENCH_PAGES, flags);
        sample_stop(&s);
        if (ret < 0) {
            printf("aligned map failed: %d\n", ret);
            break;
        }
        keep_best(&best_map_large, &s);
        tables_large = table_count(as);

        sample_start(&s);
        ret = arch_mmu_unmap(aspace, va_large, BENCH_PAGES);
        sample_stop(&s);
        if (ret < 0) {
            printf("aligned unmap failed: %d\n", ret);
            break;
        }
        keep_best(&best_unmap_large, &s);
    }

    if (ret >= 0) {
        report("map, one call", &best_map);
        report("query, per page", &best_query);
        report("unmap, one call", &best_unmap);
        report("map, one page per call", &best_map1);
        report("unmap, one page per call", &best_unmap1);
        report("map, one call, 2MB aligned", &best_map_large);
        report("unmap, one call, 2MB aligned", &best_unmap_large);
        if (tables) {
            printf("page tables after map: %zu, after the aligned map: %zu (root included)\n",
                   tables, tables_large);
        }
    }

    pmm_free(&pages);
    vmm_free_aspace(as);
    return ret < 0 ? ret : NO_ERROR;
}

STATIC_COMMAND_START
STATIC_COMMAND("mmu_bench", "time arch_mmu map/query/unmap on a private aspace", &mmu_bench)
STATIC_COMMAND_END(mmu_bench);

#endif // WITH_KERNEL_VM
