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

struct fdt_walk_callbacks {
    void (*mem)(uint64_t base, uint64_t len, void *cookie);
    void *memcookie;
    void (*cpu)(uint64_t id, void *cookie);
    void *cpucookie;
    void (*pcie)(uint64_t ecam_base, size_t len, uint8_t bus_start, uint8_t bus_end, void *cookie);
    void *pciecookie;
};

status_t fdt_walk(const void *fdt, const struct fdt_walk_callbacks *);

