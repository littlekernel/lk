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
#include <string.h>
#include <platform/interrupts.h>
#include <platform/pc.h>
#include <sys/types.h>

#if WITH_LIB_ACPI
#include <lib/acpi.h>
#endif

#define LOCAL_TRACE 0

// TODO: handle ioapics

static spin_lock_t lock;

#define INTC_TYPE_INTERNAL 0
#define INTC_TYPE_PIC      1
#define INTC_TYPE_MSI      2
#define INTC_TYPE_IOAPIC   3

struct int_vector {
    int_handler handler;
    void *arg;
    struct {
        uint allocated : 1;
        uint type      : 2; // INTC_TYPE
        uint edge      : 1; // edge vs level
        uint gsi_valid : 1;
        uint gsi       : 16;
    } flags;
};

static struct int_vector int_table[INT_VECTORS];

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

    // call the registered interrupt handler
    enum handler_return ret = INT_NO_RESCHEDULE;
    if (handler->handler) {
        ret = handler->handler(handler->arg);
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
    uint type = INTC_TYPE_PIC;
    if (vector < INT_VECTORS && int_table[vector].flags.type == INTC_TYPE_IOAPIC) {
        type = INTC_TYPE_IOAPIC;
    }

    register_int_handler_etc(vector, handler, arg, false, type);
}

void register_int_handler_msi(unsigned int vector, int_handler handler, void *arg, bool edge) {
    register_int_handler_etc(vector, handler, arg, edge, INTC_TYPE_MSI);
}

void platform_mask_irqs(void) {
    pic_mask_interrupts();
}

#if WITH_DEV_BUS_PCI
status_t platform_pci_int_line_to_vector(unsigned int pci_int_line, pci_location_t loc,
                                         unsigned int *vector) {
    (void)loc;

    if (!vector) {
        return ERR_INVALID_ARGS;
    }

    if (pci_int_line >= 16) {
        return ERR_INVALID_ARGS;
    }

    LTRACEF("pci_line %u\n", pci_int_line);

    pc_irq_route_t route;
    status_t err = pc_get_legacy_irq_route(pci_int_line, &route);
    if (err != NO_ERROR) {
        return err;
    }

    uint out_vector = pci_int_line + INT_BASE;
    if (out_vector >= INT_VECTORS) {
        return ERR_INVALID_ARGS;
    }

    ioapic_redir_state_t redir = {
        .gsi = route.gsi,
        .ioapic_id = route.ioapic_id,
        .vector = (uint8_t)out_vector,
        .destination_apic_id = (uint8_t)lapic_get_apic_id(),
        .masked = false,
        .level_triggered = route.route_level_triggered,
        .active_low = route.route_active_low,
    };

    err = ioapic_set_redir_state(route.gsi, &redir);
    if (err != NO_ERROR) {
        return err;
    }

    arch_interrupt_saved_state_t state = spin_lock_irqsave(&lock);
    int_table[out_vector].flags.allocated = true;
    int_table[out_vector].flags.type = INTC_TYPE_IOAPIC;
    int_table[out_vector].flags.edge = !route.route_level_triggered;
    int_table[out_vector].flags.gsi_valid = true;
    int_table[out_vector].flags.gsi = route.gsi;
    spin_unlock_irqrestore(&lock, state);

    dprintf(ALWAYS,
            "PC: routed legacy irq %u -> gsi %u -> vector %#x (dest apic %u, %s/%s)\n",
            pci_int_line,
            route.gsi,
            out_vector,
            redir.destination_apic_id,
            route.route_level_triggered ? "level" : "edge",
            route.route_active_low ? "low" : "high");

    *vector = out_vector;
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
    if (align_log2 > 0) {
        PANIC_UNIMPLEMENTED;
    }

    arch_interrupt_saved_state_t state = spin_lock_irqsave(&lock);

    // find a free interrupt
    status_t err = ERR_NOT_FOUND;
    for (unsigned int i = 0; i < INT_VECTORS; i++) {
        if (!int_table[i].flags.allocated) {
            int_table[i].flags.allocated = true;
            *vector = i;
            LTRACEF("found irq %#x\n", i);
            err = NO_ERROR;
            break;
        }
    }

    spin_unlock_irqrestore(&lock, state);

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
#if WITH_LIB_ACPI
static void io_apic_callback(const struct acpi_entry_hdr *hdr, void *cookie) {
    const struct acpi_madt_ioapic *entry = (const struct acpi_madt_ioapic *)hdr;

    static int index = 0;
    ioapic_init(index++, entry->address, entry->id,
                entry->gsi_base);
}
#endif

void platform_init_interrupts_postvm(void) {
#if WITH_SMP
    // Bring up the local apic on the first cpu
    // Doesn't need ACPI to detect its presence
    lapic_init_postvm();
#endif

#if WITH_LIB_ACPI
    // Now that we've scanned ACPI, try to initialize the ioapic(s)
    acpi_process_madt_entries(ACPI_MADT_ENTRY_TYPE_IOAPIC, &io_apic_callback, NULL);
#endif
}

STATIC_COMMAND_START
STATIC_COMMAND("ioapic", "dump ioapic redirection state; ioapic irq <n>; ioapic gsi <n>",
               &cmd_ioapic)
STATIC_COMMAND("irqroute", "show legacy irq to gsi/ioapic route", &cmd_irqroute)
STATIC_COMMAND_END(pc_interrupts);
