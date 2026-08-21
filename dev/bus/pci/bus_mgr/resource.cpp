/*
 * Copyright (c) 2021 Travis Geiseblrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include "resource.h"

#include <dev/bus/pci.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <lk/pow2.h>
#include <lk/compiler.h>
#include <stdlib.h>
#include <stdint.h>

#define LOCAL_TRACE 0

namespace pci {

// range_pool

range_pool::~range_pool() {
    range *r, *temp;
    list_for_every_entry_safe(&free_, r, temp, range, node) {
        list_delete(&r->node);
        delete r;
    }
}

status_t range_pool::add(uint64_t base, uint64_t size) {
    LTRACEF("base %#llx size %#llx\n", base, size);

    if (size == 0) {
        return ERR_INVALID_ARGS;
    }
    if (base + size - 1 < base) {
        // wraps around
        return ERR_INVALID_ARGS;
    }

    const uint64_t end = base + size - 1;

    // find the insertion point: the first range that starts after us
    range *next = nullptr;
    range *prev = nullptr;
    range *r;
    list_for_every_entry(&free_, r, range, node) {
        if (r->base > base) {
            next = r;
            break;
        }
        prev = r;
    }

    // reject overlaps with either neighbor
    if (prev && prev->end() >= base) {
        return ERR_ALREADY_EXISTS;
    }
    if (next && next->base <= end) {
        return ERR_ALREADY_EXISTS;
    }

    // merge into the previous range if adjacent
    if (prev && prev->end() + 1 == base) {
        prev->size += size;
        // and swallow the next one if now adjacent
        if (next && prev->end() + 1 == next->base) {
            prev->size += next->size;
            list_delete(&next->node);
            delete next;
        }
        return NO_ERROR;
    }

    // merge into the next range if adjacent
    if (next && end + 1 == next->base) {
        next->base = base;
        next->size += size;
        return NO_ERROR;
    }

    // otherwise insert a new node
    r = new range;
    r->base = base;
    r->size = size;
    if (next) {
        list_add_before(&next->node, &r->node);
    } else {
        list_add_tail(&free_, &r->node);
    }
    return NO_ERROR;
}

status_t range_pool::reserve(uint64_t base, uint64_t size) {
    LTRACEF("base %#llx size %#llx\n", base, size);

    if (size == 0) {
        return ERR_INVALID_ARGS;
    }
    if (base + size - 1 < base) {
        return ERR_INVALID_ARGS;
    }

    const uint64_t end = base + size - 1;

    range *r, *temp;
    list_for_every_entry_safe(&free_, r, temp, range, node) {
        if (r->end() < base) {
            continue;
        }
        if (r->base > end) {
            break; // sorted, nothing else can overlap
        }

        // r overlaps [base, end]
        if (r->base >= base && r->end() <= end) {
            // completely covered, drop it
            list_delete(&r->node);
            delete r;
        } else if (r->base < base && r->end() > end) {
            // reservation is in the middle, split
            range *tail = new range;
            tail->base = end + 1;
            tail->size = r->end() - end;
            r->size = base - r->base;
            list_add_after(&r->node, &tail->node);
            break;
        } else if (r->base < base) {
            // trim the tail of r
            r->size = base - r->base;
        } else {
            // trim the head of r
            const uint64_t new_base = end + 1;
            r->size = r->end() - new_base + 1;
            r->base = new_base;
        }
    }

    return NO_ERROR;
}

status_t range_pool::alloc(uint64_t size, uint8_t align_log2, uint64_t min_addr, uint64_t max_addr,
                           uint64_t *out) {
    LTRACEF("size %#llx align %u min %#llx max %#llx\n", size, align_log2, min_addr, max_addr);

    if (size == 0 || align_log2 >= 64) {
        return ERR_INVALID_ARGS;
    }

    const uint64_t align = 1ULL << align_log2;

    range *r;
    list_for_every_entry(&free_, r, range, node) {
        // lowest candidate address in this range
        uint64_t start = r->base;
        if (start < min_addr) {
            start = min_addr;
        }
        uint64_t aligned = ROUNDUP(start, align);
        if (aligned < start) {
            continue; // overflow
        }
        if (aligned > r->end() || aligned > max_addr) {
            continue;
        }

        const uint64_t last = aligned + size - 1;
        if (last < aligned) {
            continue; // overflow
        }
        if (last > r->end() || last > max_addr) {
            continue;
        }

        // fits. carve it out.
        *out = aligned;
        return reserve(aligned, size);
    }

    return ERR_NO_RESOURCES;
}

uint64_t range_pool::free_bytes() const {
    uint64_t total = 0;
    const range *r;
    list_for_every_entry(&free_, r, range, node) {
        total += r->size;
    }
    return total;
}

void range_pool::dump(const char *name) const {
    printf("  %s:", name);
    if (empty()) {
        printf(" (empty)");
    }
    const range *r;
    list_for_every_entry(&free_, r, range, node) {
        printf(" [%#llx, %#llx]", r->base, r->end());
    }
    printf("\n");
}

// resource_allocator

range_pool &resource_allocator::type_to_pool(pci_resource_type type, bool prefetchable) {
    return const_cast<range_pool &>(
        static_cast<const resource_allocator *>(this)->type_to_pool(type, prefetchable));
}

const range_pool &resource_allocator::type_to_pool(pci_resource_type type, bool prefetchable) const {
    if (prefetchable) {
        switch (type) {
            case PCI_RESOURCE_MMIO_RANGE:
                return mmio_prefetchable_;
            case PCI_RESOURCE_MMIO64_RANGE:
                return mmio64_prefetchable_;
            default:
                DEBUG_ASSERT_MSG(0, "uhandled prefetchable pci resource type %d\n", type);
        }
    } else {
        switch (type) {
            case PCI_RESOURCE_IO_RANGE:
                return io_;
            case PCI_RESOURCE_MMIO_RANGE:
                return mmio_;
            case PCI_RESOURCE_MMIO64_RANGE:
                return mmio64_;
            default:
                DEBUG_ASSERT_MSG(0, "uhandled pci resource type %d\n", type);
        }
    }
    return io_;
}

status_t resource_allocator::add_range(pci_resource_type type, bool prefetchable, uint64_t base, uint64_t size) {
    LTRACEF("type %d prefetchable %d base %#llx size %#llx\n", type, prefetchable, base, size);

    if (type == PCI_RESOURCE_IO_RANGE && prefetchable) {
        return ERR_INVALID_ARGS;
    }

    // a mmio window that fits below 4GB is 32bit, regardless of what the caller said
    if (type == PCI_RESOURCE_MMIO64_RANGE && base + size - 1 < (1ULL << 32)) {
        type = PCI_RESOURCE_MMIO_RANGE;
    }
    // and vice versa
    if (type == PCI_RESOURCE_MMIO_RANGE && base + size - 1 >= (1ULL << 32)) {
        type = PCI_RESOURCE_MMIO64_RANGE;
    }

    return type_to_pool(type, prefetchable).add(base, size);
}

status_t resource_allocator::reserve(pci_resource_type type, uint64_t base, uint64_t size) {
    LTRACEF("type %d base %#llx size %#llx\n", type, base, size);

    if (type == PCI_RESOURCE_IO_RANGE) {
        return io_.reserve(base, size);
    }

    // could be in any of the mmio pools, carve it out of all of them
    mmio_.reserve(base, size);
    mmio64_.reserve(base, size);
    mmio_prefetchable_.reserve(base, size);
    mmio64_prefetchable_.reserve(base, size);
    return NO_ERROR;
}

bool resource_allocator::has_range(pci_resource_type type, bool prefetchable) const {
    return !type_to_pool(type, prefetchable).empty();
}

status_t resource_allocator::allocate_io(uint32_t size, uint8_t align, uint32_t *out) {
    LTRACEF("size %#x align %u\n", size, align);

    // never hand out io port 0, drivers and our own unassigned check treat that as invalid.
    // prefer to stay above the legacy ISA range as well.
    static const uint64_t io_min[] = { 0x1000, 0x10 };
    for (auto min : io_min) {
        uint64_t addr;
        status_t err = io_.alloc(size, align, min, UINT32_MAX, &addr);
        if (err == NO_ERROR) {
            *out = addr;
            return NO_ERROR;
        }
    }

    return ERR_NO_RESOURCES;
}

status_t resource_allocator::allocate_mmio(bool can_be_64bit, bool prefetchable, uint64_t size, uint8_t align, uint64_t *out) {
    LTRACEF("size %#llx align %u prefetchable %d can_be_64 %d\n", size, align, prefetchable, can_be_64bit);

    // a 32bit kernel can't reach anything above 4GB
    if (sizeof(paddr_t) < 8) {
        can_be_64bit = false;
    }

    // build the list of pools to try, in order of preference: prefetchable pools first if the
    // request is, falling back to non prefetchable pools. non prefetchable requests only fall
    // back to prefetchable pools on a root allocator, where the attribute is only a hint.
    // 64bit pools are tried first for 64bit capable requests, otherwise last and capped to
    // the part of them that sits below 4GB (a pool that straddles 4GB lives there).
    struct candidate {
        range_pool *pool;
        uint64_t max_addr;
    };
    candidate candidates[8];
    size_t count = 0;

    const uint64_t max32 = (1ULL << 32) - 1;
    const uint64_t max64 = can_be_64bit ? UINT64_MAX : max32;
    auto add_pair = [&](range_pool *pool32, range_pool *pool64) {
        if (can_be_64bit) {
            candidates[count++] = { pool64, max64 };
            candidates[count++] = { pool32, max32 };
        } else {
            candidates[count++] = { pool32, max32 };
            candidates[count++] = { pool64, max64 };
        }
    };

    if (prefetchable) {
        add_pair(&mmio_prefetchable_, &mmio64_prefetchable_);
        add_pair(&mmio_, &mmio64_);
    } else {
        add_pair(&mmio_, &mmio64_);
        if (is_root_) {
            add_pair(&mmio_prefetchable_, &mmio64_prefetchable_);
        }
    }

    // prefer to stay out of the low 16MB (legacy VGA/ISA holes on a pc show up as windows
    // there), and never hand out address 0, which reads as unassigned. two passes: first with
    // the preferred minimum across every pool, then anything at all.
    static const uint64_t mmio_min[] = { 0x1000000, 1 };
    for (auto min : mmio_min) {
        for (size_t i = 0; i < count; i++) {
            status_t err = candidates[i].pool->alloc(size, align, min, candidates[i].max_addr, out);
            if (err == NO_ERROR) {
                return NO_ERROR;
            }
        }
    }

    return ERR_NO_RESOURCES;
}

void resource_allocator::dump() const {
    printf("resource allocator %p%s:\n", this, is_root_ ? " (root)" : "");
    io_.dump("io");
    mmio_.dump("mmio");
    mmio64_.dump("mmio64");
    mmio_prefetchable_.dump("mmio prefetchable");
    mmio64_prefetchable_.dump("mmio64 prefetchable");
}

} // namespace pci
