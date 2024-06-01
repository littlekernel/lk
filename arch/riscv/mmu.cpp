/*
 * Copyright (c) 2020 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#if RISCV_MMU

#include "arch/riscv/mmu.h"

#include <assert.h>
#include <string.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <arch/ops.h>
#include <arch/mmu.h>
#include <arch/riscv.h>
#include <arch/riscv/csr.h>
#include <arch/riscv/sbi.h>
#include <kernel/vm.h>

#include "riscv_priv.h"

#define LOCAL_TRACE 0

#if __riscv_xlen == 32
#error "32 bit mmu not supported yet"
#endif

// global, generally referenced in start.S

// the one main kernel top page table, used by the kernel address space
// when no user space is active. bottom user space parts are empty.
riscv_pte_t kernel_pgtable[512] __ALIGNED(PAGE_SIZE);
paddr_t kernel_pgtable_phys; // filled in by start.S

// trampoline top level page table is like the kernel page table but additionally
// holds an identity map of the bottom RISCV_MMU_PHYSMAP_SIZE bytes of ram.
// used at early bootup and when starting secondary processors.
riscv_pte_t trampoline_pgtable[512] __ALIGNED(PAGE_SIZE);
paddr_t trampoline_pgtable_phys; // filled in by start.S

// pre-allocate kernel 2nd level page tables.
// this makes it very easy to keep user space top level address space page tables
// in sync, since they can simply take a copy of the kernel ones.
riscv_pte_t kernel_l2_pgtable[512][RISCV_MMU_KERNEL_PT_ENTRIES] __ALIGNED(PAGE_SIZE);
paddr_t kernel_l2_pgtable_phys; // filled in by start.S

// initial memory mappings. VM uses to construct mappings after the fact
struct mmu_initial_mapping mmu_initial_mappings[] = {
    // all of memory, mapped in start.S
    {
        .phys = 0,
        .virt = RISCV_MMU_PHYSMAP_BASE_VIRT,
        .size = RISCV_MMU_PHYSMAP_SIZE,
        .flags = 0,
        .name = "memory"
    },

    // null entry to terminate the list
    { }
};

namespace {

// local state
ulong riscv_asid_mask;
arch_aspace_t *kernel_aspace;

// given a va address and the level, compute the index in the current PT
constexpr uint vaddr_to_index(vaddr_t va, uint level) {
    // levels count down from PT_LEVELS - 1
    DEBUG_ASSERT(level < RISCV_MMU_PT_LEVELS);

    // canonicalize the address
    va &= RISCV_MMU_CANONICAL_MASK;

    uint index = ((va >> PAGE_SIZE_SHIFT) >> (level * RISCV_MMU_PT_SHIFT)) & (RISCV_MMU_PT_ENTRIES - 1);
    LTRACEF_LEVEL(3, "canonical va %#lx, level %u = index %#x\n", va, level, index);

    return index;
}

uintptr_t constexpr page_size_per_level(uint level) {
    // levels count down from PT_LEVELS - 1
    DEBUG_ASSERT(level < RISCV_MMU_PT_LEVELS);

    return 1UL << (PAGE_SIZE_SHIFT + level * RISCV_MMU_PT_SHIFT);
}

uintptr_t constexpr page_mask_per_level(uint level) {
    return page_size_per_level(level) - 1;
}

// compute the starting and stopping index of the kernel aspace
constexpr uint kernel_start_index = vaddr_to_index(KERNEL_ASPACE_BASE, RISCV_MMU_PT_LEVELS - 1);
constexpr uint kernel_end_index = vaddr_to_index(KERNEL_ASPACE_BASE + KERNEL_ASPACE_SIZE - 1UL, RISCV_MMU_PT_LEVELS - 1);

static_assert(kernel_end_index >= kernel_start_index && kernel_end_index < RISCV_MMU_PT_ENTRIES, "");
static_assert(kernel_end_index - kernel_start_index + 1 == RISCV_MMU_KERNEL_PT_ENTRIES, "");

void riscv_set_satp(uint asid, paddr_t pt) {
    ulong satp;

#if RISCV_MMU == 48
    satp = RISCV_SATP_MODE_SV48 << RISCV_SATP_MODE_SHIFT;
#elif RISCV_MMU == 39
    satp = RISCV_SATP_MODE_SV39 << RISCV_SATP_MODE_SHIFT;
#endif

    // make sure the asid is in range
    DEBUG_ASSERT_MSG((asid & riscv_asid_mask) == asid, "asid %#x mask %#lx\n", asid, riscv_asid_mask);
    satp |= (ulong)asid << RISCV_SATP_ASID_SHIFT;

    // make sure the page table is aligned
    DEBUG_ASSERT(IS_PAGE_ALIGNED(pt));
    satp |= pt >> PAGE_SIZE_SHIFT;

    riscv_csr_write(RISCV_CSR_SATP, satp);

    // TODO: TLB flush here or use asid properly
    asm("sfence.vma zero, zero");
}

void riscv_tlb_flush_vma_range(vaddr_t base, size_t count) {
    if (count == 0)
        return;

    // Use SBI to shoot down a range of vaddrs on all the cpus
    ulong hart_mask = -1; // TODO: be more selective about the cpus
    sbi_rfence_vma(&hart_mask, base, count * PAGE_SIZE);

    // locally shoot down
    // XXX: is this needed or does the sbi call do it if included in the local hart mask?
    while (count > 0) {
        asm volatile("sfence.vma %0, zero" :: "r"(base));
        base += PAGE_SIZE;
        count--;
    }
}

void riscv_tlb_flush_global() {
    // Use SBI to do a global TLB shoot down on all cpus
    ulong hart_mask = -1; // TODO: be more selective about the cpus
    sbi_rfence_vma(&hart_mask, 0, -1);
}

volatile riscv_pte_t *alloc_ptable(arch_aspace_t *aspace, addr_t *pa) {
    // grab a page from the pmm
    vm_page_t *p = pmm_alloc_page();
    if (!p) {
        return NULL;
    }

    // get the physical and virtual mappings of the page
    *pa = vm_page_to_paddr(p);
    riscv_pte_t *pte = (riscv_pte_t *)paddr_to_kvaddr(*pa);

    // zero it out
    memset(pte, 0, PAGE_SIZE);

    smp_wmb();

    // add it to the aspace list
    list_add_head(&aspace->pt_list, &p->node);

    LTRACEF_LEVEL(3, "returning pa %#lx, va %p\n", *pa, pte);
    return pte;
}

riscv_pte_t mmu_flags_to_pte(uint flags) {
    riscv_pte_t pte = 0;

    pte |= (flags & ARCH_MMU_FLAG_PERM_USER) ? RISCV_PTE_U : 0;
    pte |= (flags & ARCH_MMU_FLAG_PERM_RO) ? RISCV_PTE_R : (RISCV_PTE_R | RISCV_PTE_W);
    pte |= (flags & ARCH_MMU_FLAG_PERM_NO_EXECUTE) ? 0 : RISCV_PTE_X;

    return pte;
}

uint pte_flags_to_mmu_flags(riscv_pte_t pte) {
    uint f = 0;
    if ((pte & (RISCV_PTE_R | RISCV_PTE_W)) == RISCV_PTE_R) {
        f |= ARCH_MMU_FLAG_PERM_RO;
    }
    f |= (pte & RISCV_PTE_X) ? 0 : ARCH_MMU_FLAG_PERM_NO_EXECUTE;
    f |= (pte & RISCV_PTE_U) ? ARCH_MMU_FLAG_PERM_USER : 0;
    return f;
}

} // namespace

// public api

// initialize per address space
status_t arch_mmu_init_aspace(arch_aspace_t *aspace, vaddr_t base, size_t size, uint flags) {
    LTRACEF("aspace %p, base %#lx, size %#zx, flags %#x\n", aspace, base, size, flags);

    DEBUG_ASSERT(aspace);
    DEBUG_ASSERT(aspace->magic != RISCV_ASPACE_MAGIC);

    // validate that the base + size is sane and doesn't wrap
    DEBUG_ASSERT(size > PAGE_SIZE);
    DEBUG_ASSERT(base + size - 1 > base);

    aspace->magic = RISCV_ASPACE_MAGIC;
    aspace->flags = flags;
    list_initialize(&aspace->pt_list);
    if (flags & ARCH_ASPACE_FLAG_KERNEL) {
        // kernel aspace is special and should be constructed once
        DEBUG_ASSERT(base == KERNEL_ASPACE_BASE);
        DEBUG_ASSERT(size == KERNEL_ASPACE_SIZE);
        DEBUG_ASSERT(!kernel_aspace);

        aspace->base = base;
        aspace->size = size;
        aspace->pt_virt = kernel_pgtable;
        aspace->pt_phys = kernel_pgtable_phys;
        kernel_aspace = aspace;

        // TODO: allocate and attach kernel page tables here instead of prealloced
    } else {
        // at the moment can only deal with user aspaces that perfectly
        // cover the predefined range
        DEBUG_ASSERT(base == USER_ASPACE_BASE);
        DEBUG_ASSERT(size == USER_ASPACE_SIZE);

        aspace->base = base;
        aspace->size = size;

        // allocate a top level page table
        aspace->pt_virt = alloc_ptable(aspace, &aspace->pt_phys);
        if (!aspace->pt_virt) {
            aspace->magic = 0; // not a properly constructed aspace
            return ERR_NO_MEMORY;
        }

        // copy the top part of the top page table from the kernel's
        for (auto i = kernel_start_index; i <= kernel_end_index; i++) {
            aspace->pt_virt[i] = kernel_pgtable[i];
        }
        smp_wmb();
    }

    LTRACEF("pt phys %#lx, pt virt %p\n", aspace->pt_phys, aspace->pt_virt);

    return NO_ERROR;
}

status_t arch_mmu_destroy_aspace(arch_aspace_t *aspace) {
    LTRACEF("aspace %p\n", aspace);

    DEBUG_ASSERT(aspace);
    DEBUG_ASSERT(aspace->magic == RISCV_ASPACE_MAGIC);

    if (aspace->flags & ARCH_ASPACE_FLAG_KERNEL) {
        panic("trying to destroy kernel aspace\n");
    } else {
        // TODO: assert that it's not active
        // TODO: shoot down the ASID

        // mass free all of the page tables in the aspace
        DEBUG_ASSERT(!list_is_empty(&aspace->pt_list)); // should be at least one page
        LTRACEF("freeing %zu page tables\n", list_length(&aspace->pt_list));
        pmm_free(&aspace->pt_list);

        // free the top level page table
        aspace->pt_virt = nullptr;
        aspace->pt_phys = 0;
    }

    aspace->magic = 0;

    return NO_ERROR;
}

namespace {

enum class walk_cb_ret_op {
    HALT,
    RESTART,
    ALLOC_PT
};

struct walk_cb_ret {
    static walk_cb_ret OpHalt(int err) { return { walk_cb_ret_op::HALT, err, false, 0, false }; }
    static walk_cb_ret OpRestart() { return { walk_cb_ret_op::RESTART, NO_ERROR, false, 0, false }; }
    static walk_cb_ret OpCommitHalt(riscv_pte_t pte, bool unmap, int err) { return { walk_cb_ret_op::HALT, err, true, pte, unmap }; }
    static walk_cb_ret OpCommitRestart(riscv_pte_t pte, bool unmap) { return { walk_cb_ret_op::RESTART, NO_ERROR, true, pte, unmap }; }
    static walk_cb_ret OpAllocPT() { return { walk_cb_ret_op::ALLOC_PT, 0, false, 0, false }; }

    // overall continuation op
    walk_cb_ret_op op;

    // if halting, return error
    int err;

    // commit the pte entry
    bool commit;
    riscv_pte_t new_pte;
    bool unmap; // we are unmapping, so test for empty page tables
};


// in the callback arg, define a function or lambda that matches this signature
using page_walk_cb = walk_cb_ret(*)(uint level, uint index, riscv_pte_t pte, vaddr_t *vaddr);

// generic walker routine to automate drilling through a page table structure
template <typename F>
int riscv_pt_walk(arch_aspace_t *aspace, vaddr_t vaddr, F callback) {
    LTRACEF("vaddr %#lx\n", vaddr);

    DEBUG_ASSERT(aspace);

restart:
    // bootstrap the top level walk
    uint level = RISCV_MMU_PT_LEVELS - 1;
    uint index = vaddr_to_index(vaddr, level);
    volatile riscv_pte_t *ptep = aspace->pt_virt + index;

    for (;;) {
        LTRACEF_LEVEL(2, "level %u, index %u, pte %p (%#lx) va %#lx\n",
                      level, index, ptep, *ptep, vaddr);

        // look at our page table entry
        riscv_pte_t pte = *ptep;
        if ((pte & RISCV_PTE_V) && !(pte & RISCV_PTE_PERM_MASK)) {
            // next level page table pointer (RWX = 0)
            paddr_t ptp = RISCV_PTE_PPN(pte);
            volatile riscv_pte_t *ptv = (riscv_pte_t *)paddr_to_kvaddr(ptp);

            LTRACEF_LEVEL(2, "next level page table at %p, pa %#lx\n", ptv, ptp);

            // go one level deeper
            level--;
            index = vaddr_to_index(vaddr, level);
            ptep = ptv + index;
        } else {
            // it's a non valid page entry or a valid terminal entry
            // call the callback, seeing what the user wants
            auto ret = callback(level, index, pte, &vaddr);
            switch (ret.op) {
                case walk_cb_ret_op::HALT:
                case walk_cb_ret_op::RESTART:
                    // see if we're being asked to commit a change
                    if (ret.commit) {
                        // commit the change
                        *ptep = ret.new_pte;
                        if (ret.unmap) {
                            // TODO: this was an unmap, test to see if we have emptied a page table
                        }
                    }

                    if (ret.op == walk_cb_ret_op::HALT) {
                        // stop here
                        return ret.err;
                    } else { // RESTART
                        // user should have modified vaddr or we'll probably be in a loop
                        goto restart;
                    }
                case walk_cb_ret_op::ALLOC_PT:
                    // user wants us to add a page table and continue
                    paddr_t ptp;
                    volatile riscv_pte_t *ptv = alloc_ptable(aspace, &ptp);
                    if (!ptv) {
                        return ERR_NO_MEMORY;
                    }

                    LTRACEF_LEVEL(2, "new ptable table %p, pa %#lx\n", ptv, ptp);

                    // link it in. RMW == 0 is a page table link
                    pte = RISCV_PTE_PPN_TO_PTE(ptp) | RISCV_PTE_V;
                    *ptep = pte;

                    // go one level deeper
                    level--;
                    index = vaddr_to_index(vaddr, level);
                    ptep = ptv + index;
                    break;
            }
        }

        // make sure we didn't decrement level one too many
        DEBUG_ASSERT(level < RISCV_MMU_PT_LEVELS);
    }
    // unreachable
}

} // namespace

// routines to map/unmap/query mappings per address space
int arch_mmu_map(arch_aspace_t *aspace, const vaddr_t _vaddr, paddr_t paddr, uint count, const uint flags) {
    LTRACEF("vaddr %#lx paddr %#lx count %u flags %#x\n", _vaddr, paddr, count, flags);

    DEBUG_ASSERT(aspace);
    DEBUG_ASSERT(aspace->magic == RISCV_ASPACE_MAGIC);

    if (flags & ARCH_MMU_FLAG_NS) {
        return ERR_INVALID_ARGS;
    }

    // trim the vaddr to the aspace
    if (_vaddr < aspace->base || _vaddr > aspace->base + aspace->size - 1) {
        return ERR_OUT_OF_RANGE;
    }
    // TODO: make sure _vaddr + count * PAGE_SIZE is within the address space

    if (count == 0) {
        return NO_ERROR;
    }

    // construct a local callback for the walker routine that
    // a) tells the walker to build a page table if it's not present
    // b) fills in a terminal page table entry with a page and tells the walker to start over
    auto map_cb = [&paddr, &count, aspace, flags](uint level, uint index, riscv_pte_t pte, vaddr_t *vaddr) -> walk_cb_ret {
        LTRACEF("level %u, index %u, pte %#lx, vaddr %#lx [paddr %#lx count %u flags %#x]\n",
                level, index, pte, *vaddr, paddr, count, flags);

        if ((pte & RISCV_PTE_V)) {
            // we have hit a valid pte of some kind
            // assert that it's not a page table pointer, which we shouldn't be hitting in the callback
            DEBUG_ASSERT(pte & RISCV_PTE_PERM_MASK);

            // for now, panic
            if (level > 0) {
                PANIC_UNIMPLEMENTED_MSG("terminal large page entry");
            } else {
                PANIC_UNIMPLEMENTED_MSG("terminal page entry");
            }

            return walk_cb_ret::OpHalt(ERR_ALREADY_EXISTS);
        }

        // hit an open pate table entry
        if (level > 0) {
            // level is > 0, allocate a page table here
            // TODO: optimize by allocating large page here if possible
            return walk_cb_ret::OpAllocPT();
        }

        // adding a terminal page at level 0
        riscv_pte_t temp_pte = RISCV_PTE_PPN_TO_PTE(paddr);
        temp_pte |= mmu_flags_to_pte(flags);
        temp_pte |= RISCV_PTE_A | RISCV_PTE_D | RISCV_PTE_V;
        temp_pte |= (aspace->flags & ARCH_ASPACE_FLAG_KERNEL) ? RISCV_PTE_G : 0;

        LTRACEF_LEVEL(2, "added new terminal entry: pte %#lx\n", temp_pte);

        // modify what the walker handed us
        *vaddr += PAGE_SIZE;

        // bump our state forward
        paddr += PAGE_SIZE;
        count--;

        // if we're done, tell the caller to commit our changes and either restart the walk or halt
        if (count == 0) {
            return walk_cb_ret::OpCommitHalt(temp_pte, false, NO_ERROR);
        } else {
            return walk_cb_ret::OpCommitRestart(temp_pte, false);
        }
    };

    return riscv_pt_walk(aspace, _vaddr, map_cb);
}

status_t arch_mmu_query(arch_aspace_t *aspace, const vaddr_t _vaddr, paddr_t *paddr, uint *flags) {
    LTRACEF("aspace %p, vaddr %#lx\n", aspace, _vaddr);

    DEBUG_ASSERT(aspace);
    DEBUG_ASSERT(aspace->magic == RISCV_ASPACE_MAGIC);

    // trim the vaddr to the aspace
    if (_vaddr < aspace->base || _vaddr > aspace->base + aspace->size - 1) {
        return ERR_OUT_OF_RANGE;
    }

    // construct a local callback for the walker routine that
    // a) if it hits a terminal entry construct the flags we want and halt
    // b) all other cases just halt and return ERR_NOT_FOUND
    auto query_cb = [paddr, flags](uint level, uint index, riscv_pte_t pte, vaddr_t *vaddr) -> walk_cb_ret {
        LTRACEF("level %u, index %u, pte %#lx, vaddr %#lx\n", level, index, pte, *vaddr);

        if (pte & RISCV_PTE_V) {
            // we have hit a valid pte of some kind
            // assert that it's not a page table pointer, which we shouldn't be hitting in the callback
            DEBUG_ASSERT(pte & RISCV_PTE_PERM_MASK);

            if (paddr) {
                // extract the ppn
                paddr_t pa = RISCV_PTE_PPN(pte);
                uintptr_t page_mask = page_mask_per_level(level);

                // add the va offset into the physical address
                *paddr = pa | (*vaddr & page_mask);
                LTRACEF_LEVEL(3, "raw pa %#lx, page_mask %#lx, final pa %#lx\n", pa, page_mask, *paddr);
            }

            if (flags) {
                // compute the flags
                *flags = pte_flags_to_mmu_flags(pte);
                LTRACEF_LEVEL(3, "computed flags %#x\n", *flags);
            }
            // we found our page, so stop
            return walk_cb_ret::OpHalt(NO_ERROR);
        } else {
            // couldnt find our page, stop
            return walk_cb_ret::OpHalt(ERR_NOT_FOUND);
        }
    };

    return riscv_pt_walk(aspace, _vaddr, query_cb);
}

int arch_mmu_unmap(arch_aspace_t *aspace, const vaddr_t _vaddr, const uint _count) {
    LTRACEF("vaddr %#lx count %u\n", _vaddr, _count);

    DEBUG_ASSERT(aspace);
    DEBUG_ASSERT(aspace->magic == RISCV_ASPACE_MAGIC);

    if (_count == 0) {
        return NO_ERROR;
    }
    // trim the vaddr to the aspace
    if (_vaddr < aspace->base || _vaddr > aspace->base + aspace->size - 1) {
        return ERR_OUT_OF_RANGE;
    }
    // TODO: make sure _vaddr + count * PAGE_SIZE is within the address space

    // construct a local callback for the walker routine that
    // a) if it hits a terminal 4K entry write zeros to it
    // b) if it hits an empty spot continue
    auto count = _count;
    auto unmap_cb = [&count]
        (uint level, uint index, riscv_pte_t pte, vaddr_t *vaddr) -> walk_cb_ret {
        LTRACEF("level %u, index %u, pte %#lx, vaddr %#lx\n", level, index, pte, *vaddr);

        if (pte & RISCV_PTE_V) {
            // we have hit a valid pte of some kind
            // assert that it's not a page table pointer, which we shouldn't be hitting in the callback
            DEBUG_ASSERT(pte & RISCV_PTE_PERM_MASK);

            if (level > 0) {
                PANIC_UNIMPLEMENTED_MSG("cannot handle unmapping of large page");
            }

            // zero it out, which should unmap the page
            // TODO: handle freeing upper level page tables
            // make sure we dont free kernel 2nd level pts
            *vaddr += PAGE_SIZE;
            count--;
            if (count == 0) {
                return walk_cb_ret::OpCommitHalt(0, true, NO_ERROR);
            } else {
                return walk_cb_ret::OpCommitRestart(0, true);
            }
        } else {
            // nothing here so skip forward and try the next page
            *vaddr += PAGE_SIZE;
            count--;
            if (count == 0) {
                return walk_cb_ret::OpHalt(NO_ERROR);
            } else {
                return walk_cb_ret::OpRestart();
            }
        }
    };

    int ret = riscv_pt_walk(aspace, _vaddr, unmap_cb);

    // TLB shootdown the range we've unmapped
    riscv_tlb_flush_vma_range(_vaddr, _count);

    return ret;
}

// load a new user address space context.
// aspace argument NULL should load kernel-only context
void arch_mmu_context_switch(arch_aspace_t *aspace) {
    LTRACEF("aspace %p\n", aspace);

    DEBUG_ASSERT(!aspace || aspace->magic == RISCV_ASPACE_MAGIC);

    if (!aspace) {
        // switch to the kernel address space
        riscv_set_satp(0, kernel_aspace->pt_phys);
    } else {
        riscv_set_satp(0, aspace->pt_phys);
    }

    // TODO: deal with TLB flushes.
    // for now, riscv_set_satp() does a full local TLB dump
}

bool arch_mmu_supports_nx_mappings(void) { return true; }
bool arch_mmu_supports_ns_mappings(void) { return false; }
bool arch_mmu_supports_user_aspaces(void) { return true; }

extern "C"
void riscv_mmu_init_secondaries() {
    // switch to the proper kernel pgtable, with the trampoline parts unmapped
    riscv_set_satp(0, kernel_pgtable_phys);

    // set the SUM bit so we can access user space directly (for now)
    riscv_csr_set(RISCV_CSR_XSTATUS, RISCV_CSR_XSTATUS_SUM);
}

// called once on the boot cpu during very early (single threaded) init
extern "C"
void riscv_early_mmu_init() {
    // figure out the number of support ASID bits by writing all 1s to
    // the asid field in satp and seeing which ones 'stick'
    auto satp_orig = riscv_csr_read(satp);
    auto satp = satp_orig | (RISCV_SATP_ASID_MASK << RISCV_SATP_ASID_SHIFT);
    riscv_csr_write(satp, satp);
    riscv_asid_mask = (riscv_csr_read(satp) >> RISCV_SATP_ASID_SHIFT) & RISCV_SATP_ASID_MASK;
    riscv_csr_write(satp, satp_orig);

    // install zeroed page tables to the unused portions of the kernel page tables
    for (auto i = kernel_start_index; i <= kernel_end_index; i++) {
        if ((trampoline_pgtable[i] & RISCV_PTE_V) == 0) {
            trampoline_pgtable[i] = RISCV_PTE_PPN_TO_PTE(kernel_l2_pgtable_phys + (i - kernel_start_index) * PAGE_SIZE) | RISCV_PTE_V;
        }
    }

    // copy the top parts of the kernel page table from the trampoline page table
    for (auto i = kernel_start_index; i <= kernel_end_index; i++) {
        kernel_pgtable[i] = trampoline_pgtable[i];
    }
    smp_wmb();

    // switch to the new kernel pagetable
    riscv_mmu_init_secondaries();
}

// called a bit later once on the boot cpu
extern "C"
void riscv_mmu_init() {
    dprintf(INFO, "RISCV: MMU ASID mask %#lx\n", riscv_asid_mask);
}

#endif
