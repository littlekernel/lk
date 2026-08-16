/*
 * Copyright (c) 2009 Corey Tabaka
 * Copyright (c) 2015 Intel Corporation
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
// Two controllers can deliver interrupts here: the legacy 8259 pair, and (on anything with a
// local apic and an ioapic described by ACPI) the ioapic. When the ioapic is usable it takes
// over: the PIC is masked off, the 16 legacy irqs are routed through the ioapic to the same
// vectors the PIC would have delivered them on (so drivers keep using INT_PIT and friends),
// and PCI INTx lines are routed to freshly allocated vectors, level triggered and shareable.
// Without an ioapic the PIC keeps doing what it always did.

static spin_lock_t lock;

#define INTC_TYPE_INTERNAL 0
#define INTC_TYPE_PIC      1
#define INTC_TYPE_MSI      2
#define INTC_TYPE_IOAPIC   3

// additional handlers on a shared (level triggered) vector
struct int_handler_entry {
    struct int_handler_entry *next;
    int_handler handler;
    void *arg;
};

struct int_vector {
    int_handler handler;
    void *arg;
    struct int_handler_entry *extra_handlers;
    struct {
        uint allocated : 1;
        uint type      : 2; // INTC_TYPE
        uint edge      : 1; // edge vs level
        uint gsi_valid : 1;
        uint gsi       : 16;
    } flags;
};

static struct int_vector int_table[INT_VECTORS];

// true once the ioapic has taken over from the 8259
static bool use_ioapic = false;

// the largest gsi the vector table can record
#define MAX_GSI (1u << 16)

// find the vector a gsi is routed to, 0 if none. lock must be held.
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

status_t pc_get_legacy_irq_route(uint source_irq, pc_irq_route_t *route) {
    if (!route) {
        return ERR_INVALID_ARGS;
    }

    memset(route, 0, sizeof(*route));
    route->source_irq = source_irq;
    route->gsi = source_irq;

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

    printf("usage: %s [irq <legacy_irq> | gsi <gsi>]\n", argv[0].str);
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

void platform_init_interrupts(void) {
    pic_init();

#if WITH_SMP
    lapic_init();
#endif

    // initialize all of the vectors
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

enum handler_return platform_irq(x86_iframe_t *frame);
enum handler_return platform_irq(x86_iframe_t *frame) {
    // get the current vector
    unsigned int vector = frame->vector;

    DEBUG_ASSERT(vector >= 0x20);

    struct int_vector *handler = &int_table[vector];

    // edge triggered interrupts are acked beforehand
    if (handler->flags.edge) {
        if (handler->flags.type == INTC_TYPE_MSI || handler->flags.type == INTC_TYPE_IOAPIC) {
            lapic_eoi(vector);
        } else {
            pic_eoi(vector);
        }
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

    // level triggered ack
    if (!handler->flags.edge) {
        if (handler->flags.type == INTC_TYPE_MSI || handler->flags.type == INTC_TYPE_IOAPIC) {
            lapic_eoi(vector);
        } else {
            pic_eoi(vector);
        }
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

    // vectors the ioapic delivers were set up (trigger mode, gsi) when they were routed, keep
    // that. level triggered ioapic vectors and 8259 vectors (pci lines share those in PIC mode)
    // are shareable, so a second registration chains rather than replacing the first.
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
        int_table[vector].extra_handlers = entry;
        entry = NULL;
    } else {
        int_table[vector].handler = handler;
        int_table[vector].arg = arg;
        if (int_table[vector].flags.type != INTC_TYPE_IOAPIC) {
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

// program the ioapic entry for a gsi and record it in the vector table. lock must be held.
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
        LTRACEF("gsi %u -> vector %#x (%s/%s)\n", gsi, v, level_triggered ? "level" : "edge",
                active_low ? "low" : "high");
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

    // x86/pc routes using the configured legacy IRQ line, not raw INTA-D pin.
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

// Try to detect the ioapic(s) from ACPI and initialize them
#if WITH_LIB_ACPI && !X86_LEGACY
static void io_apic_callback(const struct acpi_entry_hdr *hdr, void *cookie) {
    const struct acpi_madt_ioapic *entry = (const struct acpi_madt_ioapic *)hdr;

    static int index = 0;
    ioapic_init(index++, entry->address, entry->id,
                entry->gsi_base);
}
#endif

#if WITH_LIB_ACPI && !X86_LEGACY
// Switch the platform over from the 8259 to the ioapic(s): quiet the PIC and the lapic's ExtINT
// input, and route the legacy isa irqs through the ioapic onto the same vectors the PIC used.
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

    dprintf(INFO, "PC: routing legacy interrupts through the ioapic\n");

    // nothing gets through the PIC anymore
    pic_mask_interrupts();
    lapic_mask_lint0();

    // on systems that boot in PIC mode, tell the interrupt mode configuration register to
    // route through the ioapic instead of the 8259
    const struct acpi_madt *madt = (const struct acpi_madt *)acpi_get_table_by_sig(ACPI_MADT_SIGNATURE);
    if (madt && (madt->flags & ACPI_PCAT_COMPAT)) {
        outp(0x22, 0x70);
        outp(0x23, 0x01);
    }

    // look up where each isa irq lands (the MADT overrides), outside the lock since that walks
    // acpi tables
    pc_irq_route_t routes[16];
    bool route_valid[16] = {};
    for (uint irq = 0; irq < 16; irq++) {
        if (irq == 2) {
            continue; // the cascade
        }
        route_valid[irq] = (pc_get_legacy_irq_route(irq, &routes[irq]) == NO_ERROR);
    }

    // route every isa irq to its PIC vector, masked. drivers unmask what they use as before,
    // only now that reaches the ioapic.
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
        LTRACEF("isa irq %u -> gsi %u -> vector %#x (%s/%s)\n", irq, route->gsi, vector,
                route->route_level_triggered ? "level" : "edge",
                route->route_active_low ? "low" : "high");
    }
    use_ioapic = true;
    spin_unlock_irqrestore(&lock, state);
}
#endif

void platform_init_interrupts_postvm(void) {
#if WITH_SMP
    // Bring up the local apic on the first cpu
    // Doesn't need ACPI to detect its presence
    lapic_init_postvm();
#endif

#if WITH_LIB_ACPI && !X86_LEGACY
    // Now that we've scanned ACPI, try to initialize the ioapic(s) and switch to them
    acpi_process_madt_entries(ACPI_MADT_ENTRY_TYPE_IOAPIC, &io_apic_callback, NULL);
    ioapic_takeover();
#endif
}

STATIC_COMMAND_START
STATIC_COMMAND("ioapic", "dump ioapic redirection state; ioapic irq <n>; ioapic gsi <n>",
               &cmd_ioapic)
STATIC_COMMAND("irqroute", "show legacy irq to gsi/ioapic route", &cmd_irqroute)
STATIC_COMMAND_END(pc_interrupts);
