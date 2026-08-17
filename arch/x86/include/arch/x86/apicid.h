/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

__BEGIN_CDECLS

// The cpuid side of cpu topology: how an APIC id breaks down into SMT thread, core and
// package, plus the handful of facts about a logical cpu that can only be learned by
// running cpuid on that cpu (its full APIC id, its core type on hybrid parts, which last
// level cache it sits behind).
//
// This is deliberately just a decoder and a per-cpu record. It doesn't build a tree or
// decide anything; assembling the whole system's topology out of this, ACPI and platform
// knowledge is a separate job that will consume it.

// How the APIC id is partitioned, from the boot cpu's cpuid. Fields are widths in bits
// counted from the bottom of the id:
//
//   | package id | core id (incl. module/tile/die) | smt id |
//                ^ core_bits                        ^ smt_bits
//
// A cpu that doesn't enumerate any of this has both widths zero, so the whole APIC id is
// the package id and every cpu is its own core, which is also the right answer for a
// single-threaded part.
struct x86_apic_id_layout {
    uint8_t smt_bits;
    uint8_t core_bits; // cumulative: smt + core

    // apic_id >> llc_share_shift identifies the last level cache a cpu sits behind. Only
    // meaningful when llc_share_known.
    uint8_t llc_share_shift;
    bool llc_share_known;

    // Which leaf the widths came from, for cpuinfo.
    const char *source;
};

const struct x86_apic_id_layout *x86_get_apic_id_layout(void);

// Break an APIC id down according to the layout.
static inline uint32_t x86_apic_id_smt(const struct x86_apic_id_layout *l, uint32_t apic_id) {
    return apic_id & ((1u << l->smt_bits) - 1);
}
static inline uint32_t x86_apic_id_core(const struct x86_apic_id_layout *l, uint32_t apic_id) {
    return (apic_id & ((1u << l->core_bits) - 1)) >> l->smt_bits;
}
static inline uint32_t x86_apic_id_package(const struct x86_apic_id_layout *l,
                                           uint32_t apic_id) {
    return apic_id >> l->core_bits;
}

// The core type of a logical cpu on a heterogeneous ("hybrid") part.
enum x86_core_type {
    X86_CORE_TYPE_UNKNOWN, // not a hybrid part, or it doesn't say
    X86_CORE_TYPE_PERFORMANCE,
    X86_CORE_TYPE_EFFICIENT,
};

const char *x86_core_type_name(enum x86_core_type type);

// What one logical cpu knows about itself. Filled in by x86_cpu_ids_init_percpu() on that
// cpu; the boot cpu's is valid from x86_early_init_percpu() on, a secondary's from its
// own early init on.
struct x86_cpu_ids {
    bool valid;
    uint32_t apic_id; // full 32 bit id from cpuid, agrees with the local apic's
    uint32_t package_id;
    uint32_t core_id;
    uint32_t smt_id;
    uint32_t llc_share_id; // only meaningful if the layout's llc_share_known
    enum x86_core_type core_type;
    uint32_t native_model_id; // hybrid parts: the model of this particular core, 0 otherwise
};

// The ids of the cpu this is called on, or of any cpu by number.
const struct x86_cpu_ids *x86_get_cpu_ids(void);
const struct x86_cpu_ids *x86_get_cpu_ids_for_cpu(uint cpu_num);

// True if the layout has an SMT field and at least two active cpus share a core, i.e.
// there really are sibling threads running, not just a part that could have them.
bool x86_smt_active(void);

// Read the current cpu's full APIC id via cpuid, without touching the local apic.
uint32_t x86_read_local_apic_id(void);

// Init: layout once on the boot cpu (from feature init, needs the cpuid cache), then the
// per cpu record on every cpu.
void x86_apic_id_layout_init(void);
void x86_cpu_ids_init_percpu(void);

__END_CDECLS
