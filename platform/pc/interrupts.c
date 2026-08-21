/*
 * Copyright (c) 2009 Corey Tabaka
 * Copyright (c) 2015 Intel Corporation
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include "platform_p.h"
#include <arch/ops.h>
#include <arch/x86.h>
#include <arch/x86/apic.h>
#include <assert.h>
#include <kernel/spinlock.h>
#include <kernel/thread.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/reg.h>
#include <lk/trace.h>
#include <lk/console_cmd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <platform/interrupts.h>
#include <platform/pc.h>
#include <sys/types.h>

#if WITH_LIB_ACPI
#include <lib/acpi.h>
#endif
#if WITH_LIB_CMDLINE
#include <lib/cmdline.h>
#endif

#define LOCAL_TRACE 0

// Interrupt routing on the PC.
//
// Three different name spaces are in play here, and most of this file is about mapping
// between them:
//
//   vector  What the cpu sees: the index (0-255) into the IDT that an interrupt arrives on.
//           This is the number drivers pass to register_int_handler()/unmask_interrupt(),
//           and the index into int_table[] below. 0x00-0x1f are cpu exceptions, so hardware
//           interrupts start at INT_BASE (0x20).
//
//   irq     The classic ISA irq number (0-15), i.e. an input pin on the cascaded 8259 PIC
//           pair. The PICs are programmed so irq N arrives on vector INT_PIC1_BASE + N,
//           which is where names like INT_PIT (0x20) and INT_KEYBOARD (0x21) come from.
//
//   gsi     ACPI's "global system interrupt": a flat numbering of every ioapic input pin in
//           the system (each ioapic covers [gsi_base, gsi_base + num pins)). For most isa
//           irqs gsi == irq, but firmware may say otherwise via MADT Interrupt Source
//           Override entries -- the classic example is the PIT: irq 0 is wired to ioapic
//           pin (gsi) 2. PCI interrupts only exist in gsi space; which gsi a given device's
//           INTA-D pin ends up on comes from the ACPI _PRT (see platform/pc/pci.c).
//
// Two interrupt controllers can deliver interrupts, and exactly one of them is in charge:
//
//   8259 PIC pair   Always present (or emulated) on a PC and the boot-time default; the
//                   machine comes up in "virtual wire" mode where the PIC's output reaches
//                   the cpu through the local apic's LINT0/ExtINT pin.
//
//   ioapic(s)       Present on anything remotely modern, described by the ACPI MADT table.
//                   Required for PCI level triggered interrupt sharing and for routing
//                   interrupts to more than one cpu. Not active until the OS programs it.
//
// Boot flow: platform_init_interrupts() runs early, sets up the PIC and the vector table,
// and the system runs PIC-mode until the vm is up. platform_init_interrupts_postvm() then
// discovers the ioapics from the MADT and, if there is one (and a local apic), performs the
// takeover in ioapic_takeover(): the PIC is masked off for good, and every isa irq is routed
// through the ioapic *onto the same vector the PIC would have used*, so drivers keep using
// INT_PIT and friends without knowing which controller is live. PCI INTx lines are routed
// later (as devices allocate irqs) onto vectors from the dynamic range, level triggered and
// shareable. Without ACPI, an ioapic, or with pc.no_ioapic on the command line, the PIC just
// keeps doing what it always did.
//
// Each vector's int_table[] entry records which controller delivers it (INTC_TYPE_*), so
// mask/unmask and end-of-interrupt always know which piece of hardware to talk to.

static spin_lock_t lock;

// which piece of hardware delivers a given vector, and therefore how to mask it and where
// the end-of-interrupt has to go (the 8259 wants its own EOI, everything else acks the lapic)
#define INTC_TYPE_INTERNAL 0 // raised inside the lapic itself (timer, ipis, spurious); nothing to mask, acked at the lapic
#define INTC_TYPE_PIC      1 // delivered by the 8259 pair
#define INTC_TYPE_MSI      2 // written directly to the lapic by a device (MSI/MSI-X)
#define INTC_TYPE_IOAPIC   3 // delivered by an ioapic redirection entry

// additional handlers on a shared (level triggered) vector
struct int_handler_entry {
    struct int_handler_entry *next;
    int_handler handler;
    void *arg;
};

// per-vector bookkeeping, indexed by cpu vector number
struct int_vector {
    int_handler handler;
    void *arg;
    struct int_handler_entry *extra_handlers;
    struct {
        uint allocated : 1;  // handed out (or fixed purpose); the dynamic allocator skips these
        uint type      : 2;  // INTC_TYPE_*, decides the mask/unmask and EOI path
        uint edge      : 1;  // edge vs level triggered, decides when to EOI relative to the handler
        uint gsi_valid : 1;  // gsi below holds the ioapic input this vector is routed from
        uint no_eoi    : 1;  // never ack this vector (the lapic spurious vector)
        uint gsi       : 16;
    } flags;
};

static struct int_vector int_table[INT_VECTORS];

// true once the ioapic has taken over from the 8259
static bool use_ioapic = false;

// the largest gsi the vector table can record
#define MAX_GSI (1u << 16)

// find the vector a gsi is already routed to, 0 if none. this is what makes interrupt
// sharing work: a second device on the same gsi finds and reuses the first one's vector
// instead of programming the redirection entry a second time. lock must be held.
static uint vector_for_gsi(uint gsi) {
    for (uint i = INT_BASE; i < INT_VECTORS; i++) {
        if (int_table[i].flags.type == INTC_TYPE_IOAPIC && int_table[i].flags.gsi_valid &&
            int_table[i].flags.gsi == gsi) {
            return i;
        }
    }
    return 0;
}

#if WITH_LIB_ACPI
// Walk the MADT Interrupt Source Override entries looking for one isa irq. These entries are
// firmware's way of saying "isa irq N is not wired to ioapic pin N like you'd assume, it's
// on gsi M with this polarity/trigger". Almost every board has at least the irq 0 -> gsi 2
// entry for the PIT.
struct irq_override_lookup {
    uint source_irq;
    uint gsi;
    uint16_t flags;
    bool found;
};

static void int_source_override_callback(const struct acpi_entry_hdr *hdr, void *cookie) {
    const struct acpi_madt_interrupt_source_override *entry =
        (const struct acpi_madt_interrupt_source_override *)hdr;
    struct irq_override_lookup *lookup = cookie;

    if (lookup->found) {
        return;
    }

    // ISA bus only. PCI routing is handled elsewhere; this remaps legacy IRQ numbers to GSIs.
    if (entry->bus == 0 && entry->source == lookup->source_irq) {
        lookup->gsi = entry->gsi;
        lookup->flags = entry->flags;
        lookup->found = true;
    }
}
#endif

// Answer "where does isa irq N actually go?": the gsi it lands on (identity mapped unless a
// MADT override says otherwise), the polarity/trigger firmware declared for it, and -- if an
// ioapic already has a redirection entry for that gsi -- what that entry currently says.
status_t pc_get_legacy_irq_route(uint source_irq, pc_irq_route_t *route) {
    if (!route) {
        return ERR_INVALID_ARGS;
    }

    memset(route, 0, sizeof(*route));
    route->source_irq = source_irq;
    route->gsi = source_irq; // the default: isa irqs map 1:1 onto the first ioapic's pins

#if WITH_LIB_ACPI
    struct irq_override_lookup lookup = {
        .source_irq = source_irq,
        .gsi = source_irq,
        .flags = 0,
        .found = false,
    };

    if (acpi_process_madt_entries(ACPI_MADT_ENTRY_TYPE_INTERRUPT_SOURCE_OVERRIDE,
                                  int_source_override_callback,
                                  &lookup) == NO_ERROR &&
        lookup.found) {
        route->gsi = lookup.gsi;
        route->has_override = true;
        route->route_active_low =
            (lookup.flags & ACPI_MADT_POLARITY_MASK) == ACPI_MADT_POLARITY_ACTIVE_LOW;
        route->route_level_triggered =
            (lookup.flags & ACPI_MADT_TRIGGERING_MASK) == ACPI_MADT_TRIGGERING_LEVEL;
    }
#endif

    ioapic_redir_state_t redir;
    status_t err = ioapic_get_redir_state(route->gsi, &redir);
    if (err == NO_ERROR) {
        route->has_ioapic_redir = true;
        route->ioapic_id = redir.ioapic_id;
        route->vector = redir.vector;
        route->destination_apic_id = redir.destination_apic_id;
        route->masked = redir.masked;
        route->level_triggered = redir.level_triggered;
        route->active_low = redir.active_low;
    } else if (err != ERR_NOT_FOUND) {
        return err;
    }

    return NO_ERROR;
}

static void pc_dump_legacy_irq_route(uint source_irq) {
    pc_irq_route_t route;
    status_t err = pc_get_legacy_irq_route(source_irq, &route);
    if (err != NO_ERROR) {
        printf("pc: irq %u route lookup failed (%d)\n", source_irq, err);
        return;
    }

    printf("pc: irq %u -> gsi %u%s\n",
           source_irq,
           route.gsi,
           route.has_override ? " (madt override)" : "");

    if (!route.has_ioapic_redir) {
        printf("pc:   no ioapic redirection entry found for gsi %u\n", route.gsi);
        return;
    }

    printf("pc:   ioapic %u vec %#04x dest_apic %u trig %s pol %s mask %u\n",
           route.ioapic_id,
           route.vector,
           route.destination_apic_id,
           route.level_triggered ? "level" : "edge",
           route.active_low ? "low" : "high",
           route.masked ? 1 : 0);
}

static const char *intc_type_name(uint type) {
    switch (type) {
        case INTC_TYPE_INTERNAL: return "lapic";
        case INTC_TYPE_PIC:      return "pic";
        case INTC_TYPE_MSI:      return "msi";
        case INTC_TYPE_IOAPIC:   return "ioapic";
    }
    return "?";
}

// what a vector number means independent of what has been registered on it
static const char *vector_range_name(uint vector) {
    if (vector < INT_BASE) return "cpu exception";
    if (vector >= INT_PIC1_BASE && vector < INT_PIC1_BASE + 8) return "isa irq 0-7";
    if (vector >= INT_PIC2_BASE && vector < INT_PIC2_BASE + 8) return "isa irq 8-15";
    if (vector >= INT_DYNAMIC_START && vector <= INT_DYNAMIC_END) return "dynamic";
    return "lapic";
}

// two table entries are the same for the purposes of collapsing a run of vectors into one line
static bool int_vector_same(const struct int_vector *a, const struct int_vector *b) {
    return a->handler == b->handler && a->arg == b->arg &&
           a->extra_handlers == b->extra_handlers &&
           a->flags.allocated == b->flags.allocated && a->flags.type == b->flags.type &&
           a->flags.edge == b->flags.edge && a->flags.gsi_valid == b->flags.gsi_valid &&
           a->flags.gsi == b->flags.gsi;
}

// Dump the whole vector table 0-255. Runs of vectors whose entries are identical (which in
// practice means runs of unallocated dynamic vectors, or the unused tail of a fixed range)
// are collapsed into a single line so the interesting entries stand out.
static void dump_vector_table(void) {
    printf("vector     range          alloc type     trig  gsi   handler(s)\n");

    uint start = 0;
    struct int_vector cur;
    {
        arch_interrupt_saved_state_t state = spin_lock_irqsave(&lock);
        cur = int_table[0];
        spin_unlock_irqrestore(&lock, state);
    }

    for (uint i = 1; i <= INT_VECTORS; i++) {
        struct int_vector next = {};
        bool same = false;
        if (i < INT_VECTORS) {
            arch_interrupt_saved_state_t state = spin_lock_irqsave(&lock);
            next = int_table[i];
            spin_unlock_irqrestore(&lock, state);
            // don't merge across the named ranges so the range column stays meaningful
            same = int_vector_same(&cur, &next) &&
                   vector_range_name(start) == vector_range_name(i);
        }
        if (same) {
            continue;
        }

        // print the run [start, i)
        char vec_str[16];
        if (i - 1 == start) {
            snprintf(vec_str, sizeof(vec_str), "0x%02x", start);
        } else {
            snprintf(vec_str, sizeof(vec_str), "0x%02x-0x%02x", start, i - 1);
        }
        char gsi_str[8];
        if (cur.flags.gsi_valid) {
            snprintf(gsi_str, sizeof(gsi_str), "%u", cur.flags.gsi);
        } else {
            snprintf(gsi_str, sizeof(gsi_str), "-");
        }

        printf("%-10s %-14s %-5s %-8s %-5s %-5s ",
               vec_str, vector_range_name(start),
               cur.flags.allocated ? "yes" : "no",
               cur.flags.allocated ? intc_type_name(cur.flags.type) : "-",
               cur.flags.allocated ? (cur.flags.edge ? "edge" : "level") : "-",
               gsi_str);
        if (cur.handler) {
            printf("%p(%p)", cur.handler, cur.arg);
            for (struct int_handler_entry *e = cur.extra_handlers; e; e = e->next) {
                printf(" %p(%p)", e->handler, e->arg);
            }
        } else {
            printf("-");
        }
        printf("\n");

        start = i;
        cur = next;
    }
}

static int cmd_vectors(int argc, const console_cmd_args *argv) {
    dump_vector_table();
    return 0;
}

static int cmd_ioapic(int argc, const console_cmd_args *argv) {
    if (argc == 1) {
        ioapic_dump_redir_table();
        return 0;
    }

    if (!strcmp(argv[1].str, "irq")) {
        if (argc < 3) {
            printf("usage: %s irq <legacy_irq>\n", argv[0].str);
            return -1;
        }
        pc_dump_legacy_irq_route((uint)argv[2].u);
        return 0;
    }

    if (!strcmp(argv[1].str, "gsi")) {
        if (argc < 3) {
            printf("usage: %s gsi <gsi>\n", argv[0].str);
            return -1;
        }
        ioapic_redir_state_t state;
        status_t err = ioapic_get_redir_state((uint)argv[2].u, &state);
        if (err != NO_ERROR) {
            printf("pc: gsi %u lookup failed (%d)\n", (uint)argv[2].u, err);
            return -1;
        }
        printf("pc: gsi %u -> ioapic %u vec %#04x dest_apic %u trig %s pol %s mask %u\n",
               state.gsi,
               state.ioapic_id,
               state.vector,
               state.destination_apic_id,
               state.level_triggered ? "level" : "edge",
               state.active_low ? "low" : "high",
               state.masked ? 1 : 0);
        return 0;
    }

    if (!strcmp(argv[1].str, "vectors")) {
        dump_vector_table();
        return 0;
    }

    printf("usage: %s [irq <legacy_irq> | gsi <gsi> | vectors]\n", argv[0].str);
    return -1;
}

static int cmd_irqroute(int argc, const console_cmd_args *argv) {
    if (argc < 2) {
        printf("usage: %s <legacy_irq>\n", argv[0].str);
        return -1;
    }

    pc_dump_legacy_irq_route((uint)argv[1].u);
    return 0;
}

// Early (pre-vm) interrupt setup: the PIC is the only controller we can touch this early,
// since finding the ioapics needs ACPI and mapping them needs the vm. The system runs in
// PIC mode from here until the takeover in platform_init_interrupts_postvm().
void platform_init_interrupts(void) {
    // remap the 8259 pair away from the cpu exception range (to 0x20/0x28) and mask everything
    pic_init();

#if WITH_SMP
    // just the cpuid feature detection; the lapic itself is set up post-vm
    lapic_init();
#endif

    // initialize all of the vectors:
    // - the 16 vectors the PICs were just remapped onto start out PIC-typed; the ioapic
    //   takeover retypes them later if it happens
    // - only [INT_DYNAMIC_START, INT_DYNAMIC_END] is up for grabs by the vector allocator
    //   (MSI and PCI INTx); everything else is fixed-purpose and marked allocated
    for (int i = 0; i < INT_VECTORS; i++) {
        if (i >= INT_PIC1_BASE && i <= INT_PIC2_BASE + 8) {
            int_table[i].flags.type = INTC_TYPE_PIC;
        }
        if (i >= INT_DYNAMIC_START && i <= INT_DYNAMIC_END) {
            int_table[i].flags.allocated = false;
        } else {
            int_table[i].flags.allocated = true;
        }
    }
}

// mask/unmask dispatch on the vector's controller type, so callers don't need to know (or
// care) whether the 8259 or an ioapic is delivering their interrupt. MSI vectors have no
// controller-side mask here; devices mask at the source.
status_t mask_interrupt(unsigned int vector) {
    if (vector >= INT_VECTORS) {
        return ERR_INVALID_ARGS;
    }

    LTRACEF("vector %#x\n", vector);

    arch_interrupt_saved_state_t state = spin_lock_irqsave(&lock);

    if (int_table[vector].flags.type == INTC_TYPE_PIC) {
        pic_enable(vector, false);
    } else if (int_table[vector].flags.type == INTC_TYPE_IOAPIC &&
               int_table[vector].flags.gsi_valid) {
        ioapic_set_gsi_mask(int_table[vector].flags.gsi, true);
    }

    spin_unlock_irqrestore(&lock, state);

    return NO_ERROR;
}

status_t unmask_interrupt(unsigned int vector) {
    if (vector >= INT_VECTORS) {
        return ERR_INVALID_ARGS;
    }

    LTRACEF("vector %#x\n", vector);

    arch_interrupt_saved_state_t state = spin_lock_irqsave(&lock);

    if (int_table[vector].flags.type == INTC_TYPE_PIC) {
        pic_enable(vector, true);
    } else if (int_table[vector].flags.type == INTC_TYPE_IOAPIC &&
               int_table[vector].flags.gsi_valid) {
        ioapic_set_gsi_mask(int_table[vector].flags.gsi, false);
    }

    spin_unlock_irqrestore(&lock, state);

    return NO_ERROR;
}

// The common interrupt dispatch routine, called from the assembly glue for every hardware
// vector. Responsible for calling the handler(s) and issuing the end-of-interrupt to
// whichever controller delivered it (the lapic for ioapic/MSI vectors, the 8259 otherwise).
// Send the end-of-interrupt to whichever controller delivered this vector. Only the 8259 wants
// its own EOI; everything else (ioapic, msi, lapic-internal) is in service at the lapic. The
// spurious vector is the one exception: it never sets an ISR bit and must not be acked.
static void eoi_vector(const struct int_vector *v, unsigned int vector) {
    if (v->flags.no_eoi) {
        return;
    }
    if (v->flags.type == INTC_TYPE_PIC) {
        pic_eoi(vector);
    } else {
        lapic_eoi(vector);
    }
}

enum handler_return platform_irq(x86_iframe_t *frame);
enum handler_return platform_irq(x86_iframe_t *frame) {
    // get the current vector
    unsigned int vector = frame->vector;

    DEBUG_ASSERT(vector >= 0x20);

    struct int_vector *handler = &int_table[vector];

    // edge triggered interrupts are acked up front: the edge was already consumed, and acking
    // early lets a new edge that arrives while the handler runs be delivered rather than lost
    if (handler->flags.edge) {
        eoi_vector(handler, vector);
    }

    // call the registered interrupt handler(s)
    enum handler_return ret = INT_NO_RESCHEDULE;
    if (handler->handler) {
        ret = handler->handler(handler->arg);
    }
    for (struct int_handler_entry *e = handler->extra_handlers; e; e = e->next) {
        if (e->handler(e->arg) == INT_RESCHEDULE) {
            ret = INT_RESCHEDULE;
        }
    }

    // level triggered interrupts are acked after the handler: the line stays asserted until
    // the handler quiesces the device, and an earlier EOI would just re-deliver it immediately
    if (!handler->flags.edge) {
        eoi_vector(handler, vector);
    }

    return ret;
}

static void register_int_handler_etc(unsigned int vector, int_handler handler, void *arg, bool edge,
                                     uint type) {
    ASSERT(vector < INT_VECTORS);

    arch_interrupt_saved_state_t state = spin_lock_irqsave(&lock);

    int_table[vector].arg = arg;
    int_table[vector].handler = handler;
    int_table[vector].flags.allocated = true;
    int_table[vector].flags.edge = edge;
    int_table[vector].flags.type = type;

    spin_unlock_irqrestore(&lock, state);
}

void register_int_handler(unsigned int vector, int_handler handler, void *arg) {
    ASSERT(vector < INT_VECTORS);

    // Vectors the ioapic delivers had their trigger mode and gsi recorded when they were
    // routed; registering a handler must not clobber that (an earlier version of this code
    // reset every vector to edge, which broke the EOI ordering above for level triggered
    // lines).
    //
    // Some vectors are legitimately shared -- level triggered ioapic vectors (PCI INTx lines
    // can share a wire) and 8259 vectors (in PIC mode several PCI devices may sit on one
    // line, and a PCI line may also share with an isa device) -- so a second registration on
    // those chains onto the vector rather than replacing the first handler.
    //
    // allocate the (possibly unneeded) chain entry before taking the spinlock, since the
    // heap can't be used with interrupts off; it's freed below if it wasn't used.
    struct int_handler_entry *entry = malloc(sizeof(*entry));
    ASSERT(entry);
    entry->handler = handler;
    entry->arg = arg;

    arch_interrupt_saved_state_t state = spin_lock_irqsave(&lock);

    const bool shareable = (int_table[vector].flags.type == INTC_TYPE_IOAPIC && !int_table[vector].flags.edge) ||
                           int_table[vector].flags.type == INTC_TYPE_PIC;
    if (int_table[vector].handler && handler && shareable) {
        // shared: append to the chain
        entry->next = int_table[vector].extra_handlers;
        smp_wmb();
        int_table[vector].extra_handlers = entry;
        entry = NULL;
    } else {
        // first (or replacement) handler for this vector
        int_table[vector].handler = handler;
        int_table[vector].arg = arg;
        if (int_table[vector].flags.type != INTC_TYPE_IOAPIC) {
            // historical default for this api: a plain PIC-delivered, level-acked vector
            int_table[vector].flags.edge = false;
            int_table[vector].flags.type = INTC_TYPE_PIC;
        }
    }
    int_table[vector].flags.allocated = true;

    spin_unlock_irqrestore(&lock, state);

    free(entry);
}

void register_int_handler_msi(unsigned int vector, int_handler handler, void *arg, bool edge) {
    register_int_handler_etc(vector, handler, arg, edge, INTC_TYPE_MSI);
}

// Vectors the local apic raises on its own (timer, ipis, spurious). There is no controller to
// mask and nothing external to quiesce, so treat them as edge triggered and ack up front,
// except for the spurious vector which is never in service and must not be acked at all.
void register_int_handler_lapic(unsigned int vector, int_handler handler, void *arg, bool eoi) {
    register_int_handler_etc(vector, handler, arg, true, INTC_TYPE_INTERNAL);

    arch_interrupt_saved_state_t state = spin_lock_irqsave(&lock);
    int_table[vector].flags.no_eoi = !eoi;
    spin_unlock_irqrestore(&lock, state);
}

void platform_mask_irqs(void) {
    pic_mask_interrupts();
    ioapic_mask_all();
}

bool pc_using_ioapic(void) {
    return use_ioapic;
}

// find a free vector in the dynamic range. lock must be held.
static status_t allocate_vector_locked(unsigned int *vector) {
    for (unsigned int i = INT_DYNAMIC_START; i <= INT_DYNAMIC_END; i++) {
        if (!int_table[i].flags.allocated) {
            int_table[i].flags.allocated = true;
            *vector = i;
            return NO_ERROR;
        }
    }
    return ERR_NOT_FOUND;
}

// The one place an ioapic redirection entry gets programmed: point the entry for this gsi at
// a vector on the boot cpu with the given trigger/polarity, and record the routing in the
// vector table so mask/unmask/EOI and vector_for_gsi() can find it. lock must be held.
static status_t route_gsi_locked(uint gsi, uint vector, bool level, bool active_low, bool masked) {
    ioapic_redir_state_t redir = {
        .gsi = gsi,
        .vector = (uint8_t)vector,
        .destination_apic_id = (uint8_t)lapic_get_apic_id(),
        .masked = masked,
        .level_triggered = level,
        .active_low = active_low,
    };

    status_t err = ioapic_set_redir_state(gsi, &redir);
    if (err != NO_ERROR) {
        return err;
    }

    int_table[vector].flags.allocated = true;
    int_table[vector].flags.type = INTC_TYPE_IOAPIC;
    int_table[vector].flags.edge = !level;
    int_table[vector].flags.gsi_valid = true;
    int_table[vector].flags.gsi = gsi;

    return NO_ERROR;
}

// Public entry point for routing a gsi (used by the PCI code once ACPI's _PRT has named one):
// hand back the vector it is (now) routed to, allocating a fresh one from the dynamic range
// or sharing the vector of whoever routed this gsi first. The redirection entry starts out
// masked; the driver's unmask_interrupt() on the returned vector arms it.
status_t pc_route_gsi(unsigned int gsi, bool level_triggered, bool active_low, unsigned int *vector) {
    if (!vector) {
        return ERR_INVALID_ARGS;
    }
    if (!use_ioapic) {
        return ERR_NOT_SUPPORTED;
    }
    if (gsi >= MAX_GSI) {
        return ERR_INVALID_ARGS;
    }

    arch_interrupt_saved_state_t state = spin_lock_irqsave(&lock);

    status_t err = NO_ERROR;
    uint v = vector_for_gsi(gsi);
    const bool shared = (v != 0);
    if (v != 0) {
        // already routed, share the vector. a level triggered request wins over an edge one
        // (a pci line sharing a wire with something we thought was an isa irq), and vice
        // versa never happens on purpose, so warn.
        if (int_table[v].flags.edge && level_triggered) {
            dprintf(INFO, "PC: gsi %u already routed edge triggered to vector %#x, switching to level\n",
                    gsi, v);
            bool masked = false;
            ioapic_redir_state_t cur;
            if (ioapic_get_redir_state(gsi, &cur) == NO_ERROR) {
                masked = cur.masked;
            }
            err = route_gsi_locked(gsi, v, true, active_low, masked);
        } else if (!int_table[v].flags.edge && !level_triggered) {
            dprintf(INFO, "PC: gsi %u already routed level triggered to vector %#x, keeping level\n",
                    gsi, v);
        }
    } else {
        err = allocate_vector_locked(&v);
        if (err == NO_ERROR) {
            err = route_gsi_locked(gsi, v, level_triggered, active_low, true);
            if (err != NO_ERROR) {
                int_table[v].flags.allocated = false;
            }
        }
    }

    spin_unlock_irqrestore(&lock, state);

    if (err == NO_ERROR) {
        dprintf(SPEW, "PC: ioapic: gsi %u -> vector %#x (%s/%s)%s\n", gsi, v,
                level_triggered ? "level" : "edge", active_low ? "low" : "high",
                shared ? " [shared]" : "");
        *vector = v;
    }
    return err;
}

#if WITH_DEV_BUS_PCI
// Route a pci device's configured legacy interrupt line. Without an ioapic the line is the
// 8259 irq number, with one it's an isa style irq wire that the MADT overrides map to a gsi.
// This is the fallback for when ACPI can't tell us the routing (no _PRT); pci lines are level
// triggered active low either way.
status_t platform_pci_int_line_to_vector(unsigned int pci_int_line, pci_location_t loc,
                                         unsigned int *vector) {
    (void)loc;

    if (!vector) {
        return ERR_INVALID_ARGS;
    }

    if (pci_int_line >= 16 || pci_int_line == 2) {
        return ERR_INVALID_ARGS;
    }

    LTRACEF("pci_line %u\n", pci_int_line);

    if (!use_ioapic) {
        // the 8259 delivers it on the fixed vector, shared with whatever isa device is there
        uint out_vector = pci_int_line + INT_PIC1_BASE;
        arch_interrupt_saved_state_t state = spin_lock_irqsave(&lock);
        int_table[out_vector].flags.allocated = true;
        int_table[out_vector].flags.type = INTC_TYPE_PIC;
        spin_unlock_irqrestore(&lock, state);
        *vector = out_vector;
        return NO_ERROR;
    }

    pc_irq_route_t route;
    status_t err = pc_get_legacy_irq_route(pci_int_line, &route);
    if (err != NO_ERROR) {
        return err;
    }

    err = pc_route_gsi(route.gsi, true, true, vector);
    if (err != NO_ERROR) {
        return err;
    }

    dprintf(INFO, "PC: routed pci irq line %u -> gsi %u -> vector %#x (level/low)\n", pci_int_line,
            route.gsi, *vector);
    return NO_ERROR;
}

status_t platform_pci_int_pin_to_vector(unsigned int pci_int_pin, pci_location_t loc,
                                        unsigned int *vector) {
    (void)pci_int_pin;
    (void)loc;
    (void)vector;

    // A raw INTA-D pin number alone isn't routable on a pc: the pin-to-interrupt wiring is
    // board specific and only firmware knows it. The real routing comes from the ACPI _PRT
    // (see platform/pc/pci.c), which calls into pc_route_gsi() above; when there's no _PRT
    // the bus driver falls back to platform_pci_int_line_to_vector() with whatever firmware
    // left in the config space interrupt line register.
    return ERR_NOT_SUPPORTED;
}
#endif

status_t platform_allocate_interrupts(size_t count, uint align_log2, bool msi,
                                      unsigned int *vector) {
    LTRACEF("count %zu align %u msi %d\n", count, align_log2, msi);
    if (align_log2 > 0 || count != 1) {
        PANIC_UNIMPLEMENTED;
    }

    arch_interrupt_saved_state_t state = spin_lock_irqsave(&lock);

    // find a free interrupt
    status_t err = allocate_vector_locked(vector);

    spin_unlock_irqrestore(&lock, state);

    LTRACEF("returning %d, vector %#x\n", err, *vector);
    return err;
}

#if !X86_LEGACY
status_t platform_compute_msi_values(unsigned int vector, unsigned int cpu, bool edge,
                                     uint64_t *msi_address_out, uint16_t *msi_data_out) {

    // only handle edge triggered at the moment
    DEBUG_ASSERT(edge);

    // get the apic id for the target cpu
    x86_percpu_t *percpu = x86_get_percpu_for_cpu(cpu);
    if (!percpu) {
        return ERR_INVALID_ARGS;
    }
    uint32_t apic_id = percpu->apic_id;
    LTRACEF("vector %#x cpu %u apic_id %#x\n", vector, cpu, apic_id);

    if (apic_id > 0xff) {
        return ERR_INVALID_ARGS;
    }

    *msi_data_out = (vector & 0xff) | (0 << 15); // edge triggered
    *msi_address_out = 0xfee00000 | (apic_id << 12);

    return NO_ERROR;
}
#endif

// Called once per I/O APIC entry in the MADT: each entry gives the ioapic's mmio address and
// the base of the gsi range its pins occupy. ioapic_init() maps it, sizes it, and masks all
// of its redirection entries so nothing firmware left behind can fire.
#if WITH_LIB_ACPI && !X86_LEGACY
static void io_apic_callback(const struct acpi_entry_hdr *hdr, void *cookie) {
    const struct acpi_madt_ioapic *entry = (const struct acpi_madt_ioapic *)hdr;

    static int index = 0;
    ioapic_init(index++, entry->address, entry->id,
                entry->gsi_base);
}
#endif

#if WITH_LIB_ACPI && !X86_LEGACY
// Switch the platform over from the 8259 to the ioapic(s). Up to this point the machine has
// been running in the PIC mode it booted in; after this the ioapic delivers everything and
// the PIC is never unmasked again. The trick that keeps the rest of the platform oblivious:
// every isa irq is routed through the ioapic onto the *same vector* the PIC would have used
// (INT_PIC1_BASE + irq), so a driver doing register_int_handler(INT_KEYBOARD, ...) +
// unmask_interrupt(INT_KEYBOARD) works identically either way -- the vector's type flag is
// what steers the mask and EOI operations to the right controller.
static void ioapic_takeover(void) {
    bool disable = false;
#if WITH_LIB_CMDLINE
    cmdline_get_bool("pc.no_ioapic", &disable);
#endif
    if (disable) {
        dprintf(INFO, "PC: pc.no_ioapic set, staying on the 8259\n");
        return;
    }
    if (ioapic_count() == 0 || !lapic_is_present()) {
        return;
    }

    const struct acpi_madt *madt = (const struct acpi_madt *)acpi_get_table_by_sig(ACPI_MADT_SIGNATURE);
    const bool pcat_compat = madt && (madt->flags & ACPI_PCAT_COMPAT);

    dprintf(INFO, "PC: routing legacy interrupts through %zu ioapic(s), pcat compat %d\n",
            ioapic_count(), pcat_compat);

    // Quiet the legacy path down before opening the new one, so no interrupt is ever
    // deliverable through both controllers at once:
    // - mask all 16 PIC inputs
    // - mask the local apic's LINT0 pin, which is how the PIC's output has been reaching the
    //   cpu until now (boot-time "virtual wire" mode, ExtINT)
    pic_mask_interrupts();
    lapic_mask_lint0();

    // Boards whose MADT sets the PC-AT-compatible flag may have an IMCR (Interrupt Mode
    // Configuration Register, from the old MultiProcessor spec) physically steering the
    // interrupt lines to the 8259; writing 0x01 through the 0x22/0x23 index/data pair points
    // them at the (io)apic instead. On hardware without an IMCR the write is harmless.
    if (pcat_compat) {
        outp(0x22, 0x70);
        outp(0x23, 0x01);
    }

    // Look up where each isa irq actually lands. pc_get_legacy_irq_route() walks the MADT
    // Interrupt Source Override entries: without an override irq N sits on gsi N with the isa
    // default of edge/active-high; with one, firmware tells us the real gsi and the real
    // polarity/trigger (e.g. irq 0 -> gsi 2 for the PIT, or acpi's SCI as level/low). Done
    // outside the spinlock since it iterates acpi tables.
    pc_irq_route_t routes[16];
    bool route_valid[16] = {};
    for (uint irq = 0; irq < 16; irq++) {
        if (irq == 2) {
            continue; // irq 2 is the slave PIC cascade, nothing real lives there
        }
        route_valid[irq] = (pc_get_legacy_irq_route(irq, &routes[irq]) == NO_ERROR);
    }

    // Program a redirection entry for every isa irq, pointing at the same vector the PIC
    // would have delivered it on, but masked: drivers unmask what they use exactly as before,
    // and from now on that unmask reaches the ioapic (route_gsi_locked retypes the vector to
    // INTC_TYPE_IOAPIC, which is what steers mask/unmask/EOI there). Starting masked is safe
    // because every driver that unmasks an isa vector (the PIT in platform_init_timer, the
    // uart and keyboard in platform_init) runs after this hook.
    arch_interrupt_saved_state_t state = spin_lock_irqsave(&lock);
    for (uint irq = 0; irq < 16; irq++) {
        if (!route_valid[irq]) {
            continue;
        }
        const pc_irq_route_t *route = &routes[irq];

        const uint vector = INT_PIC1_BASE + irq;
        status_t err = route_gsi_locked(route->gsi, vector, route->route_level_triggered,
                                        route->route_active_low, true);
        if (err != NO_ERROR) {
            dprintf(INFO, "PC: failed to route isa irq %u (gsi %u) through the ioapic: %d\n", irq,
                    route->gsi, err);
            continue;
        }
        dprintf(SPEW, "PC: ioapic: isa irq %2u -> gsi %2u -> vector %#x (%s/%s)%s\n", irq,
                route->gsi, vector, route->route_level_triggered ? "level" : "edge",
                route->route_active_low ? "low" : "high",
                route->has_override ? " [madt override]" : "");
    }
    use_ioapic = true;
    spin_unlock_irqrestore(&lock, state);
}
#endif

// Second-stage interrupt setup, run at LK_INIT_LEVEL_VM once the vm and the ACPI tables are
// available (both are prerequisites for finding and mapping the apics). This is deliberately
// before platform_init_timer() and the driver init in platform_init(), so by the time anything
// registers and unmasks an interrupt the final controller is already in charge.
void platform_init_interrupts_postvm(void) {
#if WITH_SMP
    // Bring up the local apic on the first cpu
    // Doesn't need ACPI to detect its presence
    lapic_init_postvm();
#endif

#if WITH_LIB_ACPI && !X86_LEGACY
    // find the ioapic(s) in the MADT, then switch legacy interrupt delivery over to them
    acpi_process_madt_entries(ACPI_MADT_ENTRY_TYPE_IOAPIC, &io_apic_callback, NULL);
    ioapic_takeover();
#endif
}

STATIC_COMMAND_START
STATIC_COMMAND("ioapic", "dump ioapic redirection state; ioapic irq <n>; ioapic gsi <n>; ioapic vectors",
               &cmd_ioapic)
STATIC_COMMAND("vectors", "dump the cpu interrupt vector table", &cmd_vectors)
STATIC_COMMAND("irqroute", "show legacy irq to gsi/ioapic route", &cmd_irqroute)
STATIC_COMMAND_END(pc_interrupts);
