/*
 * Copyright (c) 2014 Brian Swetland
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <sys/types.h>

typedef struct dt_slice {
    u8 *data;
    u32 size;
} dt_slice_t;

struct devicetree_header {
    u32 magic;
    u32 size;
    u32 off_struct;     // offset from start to DT 'structure'
    u32 off_strings;    // offset from start to stringdata
    u32 off_reserve;    // offset from start to reserve memory map
    u32 version;
    u32 version_compat; // last compatible version
    u32 boot_cpuid;
    u32 sz_strings;     // size of stringdata
    u32 sz_struct;      // size of DT 'structure'
};

typedef struct devicetree {
    dt_slice_t top;
    dt_slice_t dt;
    dt_slice_t ds;
    struct devicetree_header hdr;
    void (*error)(const char *msg);
} devicetree_t;

typedef int (*dt_node_cb)(int depth, const char *name, void *cookie);
typedef int (*dt_prop_cb)(const char *name, u8 *data, u32 size, void *cookie);

int dt_init(devicetree_t *dt, void *data, u32 len);
int dt_walk(devicetree_t *dt, dt_node_cb ncb, dt_prop_cb pcb, void *cookie);

u32 dt_rd32(u8 *data);
void dt_wr32(u32 n, u8 *data);

