/*
 * Copyright (c) 2020 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <sys/types.h>

/*
 * A set of routines to assist with walking a Flattened Device Tree in memory
 * for interesting nodes. Uses libfdt internally.
 */

struct fdt_walk_pcie_info {
    // location of the ECAM and the pci ranges it covers
    uint64_t ecam_base;
    uint64_t ecam_len;
    uint8_t bus_start;
    uint8_t bus_end;

    // discovered io and mmio apertures
    uint64_t io_base;
    uint64_t io_base_mmio;
    uint64_t io_len;
    uint64_t mmio_base;
    uint64_t mmio_len;
    uint64_t mmio64_base;
    uint64_t mmio64_len;
};

struct fdt_walk_callbacks {
    void (*mem)(uint64_t base, uint64_t len, void *cookie);
    void *memcookie;
    void (*reserved_memory)(uint64_t base, uint64_t len, void *cookie);
    void *reserved_memory_cookie;
    void (*cpu)(uint64_t id, void *cookie);
    void *cpucookie;
    void (*pcie)(const struct fdt_walk_pcie_info *info, void *cookie);
    void *pciecookie;
};

status_t fdt_walk(const void *fdt, const struct fdt_walk_callbacks *);

