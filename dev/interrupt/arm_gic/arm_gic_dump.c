/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

// The "gic" console command: which handler each interrupt is wired to and the state the GIC
// holds for it. It only reads, takes no locks and allocates nothing, so it also runs in the
// panic shell; a handler registered while it runs can show up half written.

#include <lk/console_cmd.h>

#if WITH_LIB_CONSOLE

#include <arch/ops.h>
#include <ctype.h>
#include <inttypes.h>
#include <kernel/mp.h>
#include <lk/err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if WITH_LIB_SYMTAB
#include <lib/symtab.h>
#endif

#include "arm_gic_common.h"
#include "gic_v2m.h"

#if ARCH_ARM64
#include <arch/arm64/mp.h>

#include "gic_v3_its.h"
#include "gic_v3_lpi.h"
#endif

#define GIC_PER_CPU_INTS  32   // SGIs and PPIs, banked per cpu
#define GIC_SPECIAL_INTID 1020 // 1020-1023 are special, never a real interrupt

struct irq_state {
    bool valid; // false: a GICv2 SGI/PPI of another cpu, whose banked registers this cpu cannot see
    bool enabled;
    bool pending;
    bool active;
    bool edge;
    bool group;  // IGROUPR
    bool grpmod; // IGRPMODR, GICv3 only
    uint8_t prio;
};

static bool gic_is_v3(void) {
    return arm_gics[0].gic_revision > 2;
}

static bool bit_of(uint32_t reg, uint intid) {
    return (reg >> (intid % 32)) & 1;
}

static uint gic_lines(void) {
    uint lines = ((gicd_read(0, GICD_TYPER) & 0x1f) + 1) * 32;
    if (lines > GIC_SPECIAL_INTID) {
        lines = GIC_SPECIAL_INTID;
    }
    return MIN(lines, (uint)MAX_INT);
}

// cpus whose GIC state can be read: up, and on a GICv3 with a mapped redistributor frame
static bool cpu_readable(uint cpu) {
    if (!mp_is_cpu_active(cpu)) {
        return false;
    }
    if (gic_is_v3()) {
        const size_t stride = arm_gics[0].gicr_cpu_stride;
        return stride && (cpu + 1) * stride <= arm_gics[0].gicr_size;
    }
    return true;
}

static void read_state(uint intid, uint cpu, struct irq_state *s) {
    *s = (struct irq_state){};
    uint32_t cfg;
    if (intid < GIC_PER_CPU_INTS && gic_is_v3()) {
        s->enabled = bit_of(gicr_read(0, cpu, GICR_ISENABLER0), intid);
        s->pending = bit_of(gicr_read(0, cpu, GICR_ISPENDR0), intid);
        s->active = bit_of(gicr_read(0, cpu, GICR_ISACTIVER0), intid);
        s->group = bit_of(gicr_read(0, cpu, GICR_IGROUPR0), intid);
        s->grpmod = bit_of(gicr_read(0, cpu, GICR_IGRPMODR0), intid);
        s->prio = (uint8_t)(gicr_read(0, cpu, GICR_IPRIORITYR(intid / 4)) >> ((intid % 4) * 8));
        cfg = gicr_read(0, cpu, GICR_ICFGR(intid / 16));
    } else {
        if (intid < GIC_PER_CPU_INTS && cpu != arch_curr_cpu_num()) {
            return;
        }
        s->enabled = bit_of(gicd_read(0, GICD_ISENABLER(intid / 32)), intid);
        s->pending = bit_of(gicd_read(0, GICD_ISPENDR(intid / 32)), intid);
        s->active = bit_of(gicd_read(0, GICD_ISACTIVER(intid / 32)), intid);
        s->group = bit_of(gicd_read(0, GICD_IGROUPR(intid / 32)), intid);
        if (gic_is_v3()) {
            s->grpmod = bit_of(gicd_read(0, GICD_IGRPMODR(intid / 32)), intid);
        }
        s->prio = (uint8_t)(gicd_read(0, GICD_IPRIORITYR(intid / 4)) >> ((intid % 4) * 8));
        cfg = gicd_read(0, GICD_ICFGR(intid / 16));
    }
    s->edge = (cfg >> ((intid % 16) * 2 + 1)) & 1;
    s->valid = true;
}

static bool state_equal(const struct irq_state *a, const struct irq_state *b) {
    return a->valid == b->valid && a->enabled == b->enabled && a->pending == b->pending &&
           a->active == b->active && a->edge == b->edge && a->group == b->group &&
           a->grpmod == b->grpmod && a->prio == b->prio;
}

// An enabled SGI is not by itself worth a row: GICv2 SGIs are permanently enabled.
static bool state_interesting(uint intid, const struct irq_state *s) {
    if (!s->valid) {
        return false;
    }
    return s->pending || s->active || (s->enabled && intid >= GIC_BASE_PPI);
}

static const char *kind_name(uint intid) {
    if (intid < GIC_BASE_PPI) {
        return "SGI";
    }
    if (intid < GIC_BASE_SPI) {
        return "PPI";
    }
    if (intid < GIC_SPECIAL_INTID) {
        return "SPI";
    }
    return "LPI";
}

static const char *group_name(const struct irq_state *s) {
    if (!gic_is_v3()) {
        return s->group ? "G1" : "G0";
    }
    if (s->group) {
        return s->grpmod ? "res" : "G1NS";
    }
    return s->grpmod ? "G1S" : "G0";
}

// where an SPI is routed: a cpu, any cpu (1 of N), or a raw target
static void format_route(uint intid, char *buf, size_t len) {
    if (intid < GIC_BASE_SPI) {
        snprintf(buf, len, "-");
        return;
    }
    if (!gic_is_v3()) {
        const uint mask = (gicd_read(0, GICD_ITARGETSR(intid / 4)) >> ((intid % 4) * 8)) & 0xff;
        if (mask && !(mask & (mask - 1))) {
            snprintf(buf, len, "cpu%u", (uint)__builtin_ctz(mask));
        } else {
            snprintf(buf, len, "mask %#x", mask);
        }
        return;
    }

    const uint64_t router = gicd_read64(0, GICD_IROUTER(intid));
    if (router & (1ULL << 31)) {
        snprintf(buf, len, "any");
        return;
    }
    const uint32_t aff = (uint32_t)(router & 0xffffff) | (uint32_t)((router >> 32) & 0xff) << 24;
#if ARCH_ARM64
    for (uint cpu = 0; cpu < SMP_MAX_CPUS; cpu++) {
        if (!mp_is_cpu_active(cpu)) {
            continue;
        }
        const uint64_t mpidr = arm64_cpu_num_to_mpidr(cpu);
        if (((uint32_t)(mpidr & 0xffffff) | (uint32_t)((mpidr >> 32) & 0xff) << 24) == aff) {
            snprintf(buf, len, "cpu%u", cpu);
            return;
        }
    }
#endif
    snprintf(buf, len, "aff %#x", aff);
}

static void print_handler(const struct int_handler_struct *h) {
    if (!h->handler) {
        printf("-");
        return;
    }
#if WITH_LIB_SYMTAB
    uintptr_t addr = (uintptr_t)h->handler;
#if ARCH_ARM
    addr &= ~(uintptr_t)1; // the thumb bit
#endif
    uintptr_t base;
    const char *name = symtab_lookup(addr, &base);
    if (name) {
        printf("%s", name);
        if (addr != base) {
            printf("+%#lx", (unsigned long)(addr - base));
        }
        printf("(%p)", h->arg);
        return;
    }
#endif
    printf("%p(%p)", (void *)h->handler, h->arg);
}

static void print_header(void) {
    printf("intid kind cpu   en pnd act trig  prio grp  route    handler(arg)\n");
}

// one row; cpu_first > cpu_last means no cpu column (SPIs, LPIs)
static void print_row(uint intid, uint cpu_first, uint cpu_last, const struct irq_state *s,
                      const char *route, const struct int_handler_struct *h) {
    char cpus[8] = "";
    if (cpu_first == cpu_last) {
        snprintf(cpus, sizeof(cpus), "%u", cpu_first);
    } else if (cpu_first < cpu_last) {
        snprintf(cpus, sizeof(cpus), "%u-%u", cpu_first, cpu_last);
    }
    printf("%5u %-4s %-5s ", intid, kind_name(intid), cpus);
    if (s->valid) {
        printf("%-2s %-3s %-3s %-5s 0x%02x %-4s ", s->enabled ? "y" : ".", s->pending ? "y" : ".",
               s->active ? "y" : ".", s->edge ? "edge" : "level", s->prio, group_name(s));
    } else {
        printf("%-2s %-3s %-3s %-5s %-4s %-4s ", "?", "?", "?", "?", "?", "?");
    }
    printf("%-8s ", route);
    print_handler(h);
}

// An SGI or PPI, one row per run of consecutive cpus that agree on the handler and, on a GICv3,
// the state. A GICv2 row shows the calling cpu's state if the run includes it.
static void dump_per_cpu(uint intid, bool all) {
    const uint curr = arch_curr_cpu_num();
    struct irq_state states[SMP_MAX_CPUS];
    bool readable[SMP_MAX_CPUS];
    bool interesting = all;
    for (uint cpu = 0; cpu < SMP_MAX_CPUS; cpu++) {
        readable[cpu] = cpu_readable(cpu);
        if (!readable[cpu]) {
            continue;
        }
        read_state(intid, cpu, &states[cpu]);
        if (get_int_handler(intid, cpu)->handler || state_interesting(intid, &states[cpu])) {
            interesting = true;
        }
    }
    if (!interesting) {
        return;
    }

    for (uint cpu = 0; cpu < SMP_MAX_CPUS;) {
        if (!readable[cpu]) {
            cpu++;
            continue;
        }
        const struct int_handler_struct *h = get_int_handler(intid, cpu);
        uint last = cpu;
        while (last + 1 < SMP_MAX_CPUS && readable[last + 1] &&
               (!gic_is_v3() || state_equal(&states[cpu], &states[last + 1])) &&
               get_int_handler(intid, last + 1)->handler == h->handler &&
               get_int_handler(intid, last + 1)->arg == h->arg) {
            last++;
        }
        const struct irq_state *s = &states[cpu];
        if (!gic_is_v3() && curr >= cpu && curr <= last) {
            s = &states[curr];
        }
        print_row(intid, cpu, last, s, "-", h);
        printf("\n");
        cpu = last + 1;
    }
}

static void dump_spi(uint intid, bool all) {
    struct irq_state s;
    read_state(intid, 0, &s);
    const struct int_handler_struct *h = get_int_handler(intid, 0);
    if (!all && !h->handler && !state_interesting(intid, &s)) {
        return;
    }
    char route[16];
    format_route(intid, route, sizeof(route));
    print_row(intid, 1, 0, &s, route, h);
    paddr_t frame;
    if (arm_gic_v2m_spi_frame(intid, &frame)) {
        printf(" [msi v2m %#" PRIxPTR "]", (uintptr_t)frame);
    }
    printf("\n");
}

#if ARCH_ARM64
static uint lpi_count(void) {
    uint count = 0;
    for (uint i = 0; i < ARM_GIC_MAX_LPIS; i++) {
        count += arm_gicv3_lpi_handler(GIC_LPI_BASE + i) != NULL;
    }
    return count;
}

// LPIs have no active state, and their pending state lives in memory the redistributor may
// cache, so only the property byte is shown. The collection an LPI targets is not tracked.
static void dump_lpi(uint intid) {
    const struct int_handler_struct *h = arm_gicv3_lpi_handler(intid);
    if (!h) {
        return;
    }
    const uint8_t prop = arm_gicv3_lpi_prop(intid);
    const struct irq_state s = {
        .valid = true,
        .enabled = prop & 1,
        .edge = true,
        .group = true,
        .prio = prop & 0xfc,
    };
    print_row(intid, 1, 0, &s, "-", h);
    uint32_t device_id, event;
    if (arm_gicv3_its_lpi_source(intid, &device_id, &event)) {
        printf(" [its dev %#x ev %u]", device_id, event);
    }
    printf("\n");
}
#endif

static void dump_summary(void) {
    const uint32_t typer = gicd_read(0, GICD_TYPER);
    printf("GICv%d: %u interrupt lines, %u handled (MAX_INT), cpu %u", gic_is_v3() ? 3 : 2,
           ((typer & 0x1f) + 1) * 32, gic_lines(), arch_curr_cpu_num());
#if ARCH_ARM64
    if (gic_is_v3() && arm_gicv3_lpi_ready()) {
        printf(", %u LPIs allocated", lpi_count());
        if (arm_gicv3_its_ready()) {
            printf(", ITS at %#" PRIxPTR, (uintptr_t)arm_gicv3_its_paddr());
        }
    }
#endif
    printf("\n");
    if (!gic_is_v3()) {
        printf("SGI/PPI state is banked per cpu: rows show cpu %u's, or ? without it\n",
               arch_curr_cpu_num());
    }
}

static int cmd_gic(int argc, const console_cmd_args *argv) {
    if (!arm_gics[0].gicd_vaddr) {
        printf("GIC not initialized\n");
        return ERR_NOT_READY;
    }

    bool all = false;
    if (argc >= 2) {
        if (!strcmp(argv[1].str, "all")) {
            all = true;
        } else if (isdigit((unsigned char)argv[1].str[0])) {
            const uint intid = (uint)argv[1].u;
            if (intid < GIC_PER_CPU_INTS) {
                print_header();
                dump_per_cpu(intid, true);
                return 0;
            }
            if (intid < gic_lines()) {
                print_header();
                dump_spi(intid, true);
                return 0;
            }
#if ARCH_ARM64
            if (arm_gicv3_lpi_handler(intid)) {
                print_header();
                dump_lpi(intid);
                return 0;
            }
#endif
            printf("intid %u is not an SGI, PPI or SPI below %u, or an allocated LPI\n", intid,
                   gic_lines());
            return ERR_NOT_FOUND;
        } else {
            printf("usage: %s [all | <intid>]\n", argv[0].str);
            printf("  no argument: interrupts with a handler, or enabled, pending or active\n");
            return ERR_INVALID_ARGS;
        }
    }

    dump_summary();
    print_header();
    for (uint intid = 0; intid < GIC_PER_CPU_INTS; intid++) {
        dump_per_cpu(intid, all);
    }
    const uint lines = gic_lines();
    for (uint intid = GIC_BASE_SPI; intid < lines; intid++) {
        dump_spi(intid, all);
    }
#if ARCH_ARM64
    for (uint i = 0; i < ARM_GIC_MAX_LPIS; i++) {
        dump_lpi(GIC_LPI_BASE + i);
    }
#endif
    return 0;
}

STATIC_COMMAND_START
STATIC_COMMAND_MASKED("gic", "dump GIC interrupts: handlers, state, routing; gic all; gic <intid>",
                      &cmd_gic, CMD_AVAIL_ALWAYS)
STATIC_COMMAND_END(arm_gic);

#endif // WITH_LIB_CONSOLE
