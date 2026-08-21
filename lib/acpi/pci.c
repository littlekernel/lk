// Copyright 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

// only useful when the pci bus manager is in the build
#if WITH_DEV_BUS_PCI

#include <lib/acpi/pci.h>

#include <lk/debug.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uacpi/namespace.h>
#include <uacpi/resources.h>
#include <uacpi/uacpi.h>
#include <uacpi/utilities.h>

#define LOCAL_TRACE 0

// Host bridge enumeration

// _CRS walk state for one root
struct crs_ctx {
    struct acpi_pci_root *root;
    bool found_bus_range;
};

static void add_window(struct acpi_pci_root *root, enum pci_resource_type type, uint64_t base,
                       uint64_t len, uint64_t translation, bool prefetchable) {
    if (len == 0) {
        return;
    }
    if (root->num_windows >= ACPI_PCI_ROOT_MAX_WINDOWS) {
        printf("ACPI PCI: root %04x:%02x has too many windows, dropping one\n", root->segment,
               root->bus_start);
        return;
    }

    struct pci_root_window *w = &root->windows[root->num_windows++];
    w->type = type;
    w->base = base;
    w->size = len;
    w->translation_offset = translation;
    w->prefetchable = prefetchable;
}

// handle one address space descriptor (any of the 16/32/64 bit variants) from a host bridge _CRS
static void handle_address_descriptor(struct crs_ctx *ctx, const uacpi_resource_address_common *common,
                                      uint64_t minimum, uint64_t maximum, uint64_t translation,
                                      uint64_t length) {
    struct acpi_pci_root *root = ctx->root;

    LTRACEF("type %u direction %u min %#llx max %#llx translation %#llx len %#llx\n",
            common->type, common->direction, minimum, maximum, translation, length);

    if (length == 0 || maximum < minimum) {
        return;
    }

    switch (common->type) {
        case UACPI_RANGE_BUS:
            // the range of bus numbers this bridge owns
            if (minimum > 255) {
                return;
            }
            root->bus_start = minimum;
            root->bus_end = (maximum > 255) ? 255 : maximum;
            ctx->found_bus_range = true;
            break;
        case UACPI_RANGE_IO:
            if (common->direction != UACPI_PRODUCER) {
                return;
            }
            add_window(root, PCI_RESOURCE_IO_RANGE, minimum, length, translation, false);
            break;
        case UACPI_RANGE_MEMORY: {
            if (common->direction != UACPI_PRODUCER) {
                return;
            }
            enum pci_resource_type type =
                (maximum >= (1ULL << 32)) ? PCI_RESOURCE_MMIO64_RANGE : PCI_RESOURCE_MMIO_RANGE;
            bool prefetchable = (common->attribute.memory.caching == UACPI_PREFETCHABLE);
            add_window(root, type, minimum, length, translation, prefetchable);
            break;
        }
        default:
            break;
    }
}

static uacpi_iteration_decision root_crs_callback(void *user, uacpi_resource *res) {
    struct crs_ctx *ctx = user;

    switch (res->type) {
        case UACPI_RESOURCE_TYPE_ADDRESS16: {
            const uacpi_resource_address16 *a = &res->address16;
            handle_address_descriptor(ctx, &a->common, a->minimum, a->maximum,
                                      a->translation_offset, a->address_length);
            break;
        }
        case UACPI_RESOURCE_TYPE_ADDRESS32: {
            const uacpi_resource_address32 *a = &res->address32;
            handle_address_descriptor(ctx, &a->common, a->minimum, a->maximum,
                                      a->translation_offset, a->address_length);
            break;
        }
        case UACPI_RESOURCE_TYPE_ADDRESS64: {
            const uacpi_resource_address64 *a = &res->address64;
            handle_address_descriptor(ctx, &a->common, a->minimum, a->maximum,
                                      a->translation_offset, a->address_length);
            break;
        }
        case UACPI_RESOURCE_TYPE_ADDRESS64_EXTENDED: {
            const uacpi_resource_address64_extended *a = &res->address64_extended;
            handle_address_descriptor(ctx, &a->common, a->minimum, a->maximum,
                                      a->translation_offset, a->address_length);
            break;
        }
        default:
            // IO()/FixedIO() and memory descriptors in a host bridge _CRS describe resources the
            // bridge itself consumes (config ports and the like), not windows it produces.
            break;
    }

    return UACPI_ITERATION_DECISION_CONTINUE;
}

// accumulated roots during the namespace walk, sorted and handed out afterwards
struct enumerate_ctx {
    struct acpi_pci_root **roots;
    size_t count;
    size_t capacity;
};

static uacpi_iteration_decision found_host_bridge(void *user, uacpi_namespace_node *node,
                                                  uacpi_u32 depth) {
    struct enumerate_ctx *ectx = user;

    struct acpi_pci_root *root = calloc(1, sizeof(*root));
    if (!root) {
        return UACPI_ITERATION_DECISION_BREAK;
    }
    root->node = node;

    // segment and bus number, both optional
    uacpi_u64 val;
    if (uacpi_eval_simple_integer(node, "_SEG", &val) == UACPI_STATUS_OK) {
        root->segment = val;
    }
    if (uacpi_eval_simple_integer(node, "_BBN", &val) == UACPI_STATUS_OK) {
        root->bus_start = (val > 255) ? 255 : val;
    }
    // until _CRS says otherwise, assume the bridge owns everything above its base bus
    root->bus_end = 255;

    struct crs_ctx cctx = { .root = root, .found_bus_range = false };
    uacpi_status st = uacpi_for_each_device_resource(node, "_CRS", root_crs_callback, &cctx);
    if (st != UACPI_STATUS_OK && st != UACPI_STATUS_NOT_FOUND) {
        const char *path = uacpi_namespace_node_generate_absolute_path(node);
        printf("ACPI PCI: failed to evaluate _CRS of %s: %s\n", path ? path : "?",
               uacpi_status_to_string(st));
        uacpi_free_absolute_path(path);
    }

    // stash it
    if (ectx->count == ectx->capacity) {
        size_t new_capacity = ectx->capacity ? ectx->capacity * 2 : 4;
        struct acpi_pci_root **new_roots =
            realloc(ectx->roots, new_capacity * sizeof(*new_roots));
        if (!new_roots) {
            free(root);
            return UACPI_ITERATION_DECISION_BREAK;
        }
        ectx->roots = new_roots;
        ectx->capacity = new_capacity;
    }
    ectx->roots[ectx->count++] = root;

    return UACPI_ITERATION_DECISION_CONTINUE;
}

static int compare_roots(const void *a, const void *b) {
    const struct acpi_pci_root *ra = *(const struct acpi_pci_root *const *)a;
    const struct acpi_pci_root *rb = *(const struct acpi_pci_root *const *)b;

    if (ra->segment != rb->segment) {
        return (ra->segment < rb->segment) ? -1 : 1;
    }
    if (ra->bus_start != rb->bus_start) {
        return (ra->bus_start < rb->bus_start) ? -1 : 1;
    }
    return 0;
}

status_t acpi_pci_enumerate_roots(acpi_pci_root_callback callback, void *cookie) {
    static const char *const hids[] = { "PNP0A03", "PNP0A08", NULL };

    struct enumerate_ctx ectx = {};
    uacpi_status st = uacpi_find_devices_at(uacpi_namespace_root(), hids, found_host_bridge, &ectx);
    if (st != UACPI_STATUS_OK) {
        printf("ACPI PCI: host bridge search failed: %s\n", uacpi_status_to_string(st));
        free(ectx.roots);
        return ERR_GENERIC;
    }

    if (ectx.count == 0) {
        free(ectx.roots);
        return ERR_NOT_FOUND;
    }

    // sort by segment and starting bus, then make sure roots that only told us their base bus
    // don't run into the next one, and drop any that start on a bus a previous root owns
    qsort(ectx.roots, ectx.count, sizeof(ectx.roots[0]), compare_roots);
    for (size_t i = 0; i < ectx.count; i++) {
        struct acpi_pci_root *r = ectx.roots[i];
        struct acpi_pci_root *prev = NULL;
        for (size_t j = i; j > 0 && !prev; j--) {
            prev = ectx.roots[j - 1];
        }

        if (prev && prev->segment == r->segment && prev->bus_end >= r->bus_start) {
            if (r->bus_start == prev->bus_start) {
                // two roots claiming the same starting bus, keep the first
                printf("ACPI PCI: dropping root %04x:%02x that duplicates a previous one\n",
                       r->segment, r->bus_start);
                acpi_pci_root_free(r);
                ectx.roots[i] = NULL;
                continue;
            }
            // trim the previous root, it either guessed (no bus range in _CRS) or firmware is
            // inconsistent, either way this one starts here
            prev->bus_end = r->bus_start - 1;
        }
    }

    for (size_t i = 0; i < ectx.count; i++) {
        struct acpi_pci_root *r = ectx.roots[i];
        if (r) {
            callback(r, cookie);
        }
    }
    free(ectx.roots);

    return NO_ERROR;
}

void acpi_pci_root_free(struct acpi_pci_root *root) {
    if (!root) {
        return;
    }
    if (root->prt) {
        uacpi_free_pci_routing_table(root->prt);
    }
    free(root);
}

void acpi_pci_root_dump(const struct acpi_pci_root *root) {
    const char *path = uacpi_namespace_node_generate_absolute_path(root->node);
    printf("ACPI PCI root %s: segment %#x bus [%#x...%#x]\n", path ? path : "?", root->segment,
           root->bus_start, root->bus_end);
    uacpi_free_absolute_path(path);
    for (size_t i = 0; i < root->num_windows; i++) {
        const struct pci_root_window *w = &root->windows[i];
        printf("  window %s: [%#llx, %#llx]%s", pci_resource_type_to_str(w->type), w->base,
               w->base + w->size - 1, w->prefetchable ? " prefetchable" : "");
        if (w->translation_offset) {
            printf(" translation %#llx", w->translation_offset);
        }
        printf("\n");
    }
}

// Interrupt routing

status_t acpi_pci_set_interrupt_model(bool ioapic) {
    uacpi_status st = uacpi_set_interrupt_model(ioapic ? UACPI_INTERRUPT_MODEL_IOAPIC
                                                       : UACPI_INTERRUPT_MODEL_PIC);
    if (st != UACPI_STATUS_OK) {
        printf("ACPI PCI: _PIC failed: %s\n", uacpi_status_to_string(st));
        return ERR_GENERIC;
    }
    return NO_ERROR;
}

status_t acpi_pci_root_load_prt(struct acpi_pci_root *root) {
    if (root->prt) {
        return NO_ERROR;
    }

    uacpi_status st = uacpi_get_pci_routing_table(root->node, &root->prt);
    if (st == UACPI_STATUS_NOT_FOUND) {
        return ERR_NOT_FOUND;
    }
    if (st != UACPI_STATUS_OK) {
        printf("ACPI PCI: failed to evaluate _PRT of root %04x:%02x: %s\n", root->segment,
               root->bus_start, uacpi_status_to_string(st));
        return ERR_GENERIC;
    }

    LTRACEF("root %04x:%02x: %zu _PRT entries\n", root->segment, root->bus_start,
            root->prt->num_entries);
    return NO_ERROR;
}

// state while walking a link device's resource list looking for the interrupt it's set to
struct link_irq_ctx {
    unsigned int wanted_index; // which descriptor the _PRT pointed at
    unsigned int index;        // running descriptor count
    bool found;
    struct acpi_pci_irq irq;
};

static uacpi_iteration_decision link_irq_callback(void *user, uacpi_resource *res) {
    struct link_irq_ctx *ctx = user;
    const unsigned int this_index = ctx->index++;

    // pick the descriptor the _PRT pointed at, or failing that the first irq descriptor
    switch (res->type) {
        case UACPI_RESOURCE_TYPE_IRQ: {
            const uacpi_resource_irq *irq = &res->irq;
            if (irq->num_irqs == 0) {
                return UACPI_ITERATION_DECISION_CONTINUE;
            }
            if (ctx->found && this_index != ctx->wanted_index) {
                return UACPI_ITERATION_DECISION_CONTINUE;
            }
            ctx->irq.gsi = irq->irqs[0];
            // the short form irq descriptor without flags is edge/high per spec, but for a pci
            // interrupt link that is never right, so only trust the flags if they say level
            ctx->irq.level_triggered = (irq->length_kind == UACPI_RESOURCE_LENGTH_KIND_FULL)
                                           ? (irq->triggering == UACPI_TRIGGERING_LEVEL)
                                           : true;
            ctx->irq.active_low = (irq->length_kind == UACPI_RESOURCE_LENGTH_KIND_FULL)
                                      ? (irq->polarity == UACPI_POLARITY_ACTIVE_LOW)
                                      : true;
            ctx->found = true;
            break;
        }
        case UACPI_RESOURCE_TYPE_EXTENDED_IRQ: {
            const uacpi_resource_extended_irq *irq = &res->extended_irq;
            if (irq->num_irqs == 0) {
                return UACPI_ITERATION_DECISION_CONTINUE;
            }
            if (ctx->found && this_index != ctx->wanted_index) {
                return UACPI_ITERATION_DECISION_CONTINUE;
            }
            ctx->irq.gsi = irq->irqs[0];
            ctx->irq.level_triggered = (irq->triggering == UACPI_TRIGGERING_LEVEL);
            ctx->irq.active_low = (irq->polarity == UACPI_POLARITY_ACTIVE_LOW);
            ctx->found = true;
            break;
        }
        default:
            break;
    }

    return (ctx->found && this_index == ctx->wanted_index) ? UACPI_ITERATION_DECISION_BREAK
                                                            : UACPI_ITERATION_DECISION_CONTINUE;
}

// state while trying to program a link device that firmware left disabled
struct link_srs_ctx {
    uacpi_resource *copy;
};

static uacpi_iteration_decision link_prs_callback(void *user, uacpi_resource *res) {
    struct link_srs_ctx *ctx = user;

    if (res->type != UACPI_RESOURCE_TYPE_IRQ && res->type != UACPI_RESOURCE_TYPE_EXTENDED_IRQ) {
        return UACPI_ITERATION_DECISION_CONTINUE;
    }

    // take a copy of the first irq descriptor and trim it to its first possible irq
    uacpi_resource *copy = malloc(res->length);
    if (!copy) {
        return UACPI_ITERATION_DECISION_BREAK;
    }
    memcpy(copy, res, res->length);
    if (copy->type == UACPI_RESOURCE_TYPE_IRQ) {
        if (copy->irq.num_irqs == 0) {
            free(copy);
            return UACPI_ITERATION_DECISION_CONTINUE;
        }
        copy->irq.num_irqs = 1;
    } else {
        if (copy->extended_irq.num_irqs == 0) {
            free(copy);
            return UACPI_ITERATION_DECISION_CONTINUE;
        }
        copy->extended_irq.num_irqs = 1;
    }
    ctx->copy = copy;
    return UACPI_ITERATION_DECISION_BREAK;
}

// firmware left the link device with no interrupt selected. pick the first one it says is
// possible and program it with _SRS.
static status_t link_enable(uacpi_namespace_node *link) {
    struct link_srs_ctx ctx = {};
    uacpi_status st = uacpi_for_each_device_resource(link, "_PRS", link_prs_callback, &ctx);
    if (st != UACPI_STATUS_OK || !ctx.copy) {
        return ERR_NOT_FOUND;
    }

    uacpi_resources resources = {
        .length = ctx.copy->length,
        .entries = ctx.copy,
    };
    st = uacpi_set_resources(link, &resources);
    free(ctx.copy);
    if (st != UACPI_STATUS_OK) {
        printf("ACPI PCI: _SRS on interrupt link failed: %s\n", uacpi_status_to_string(st));
        return ERR_GENERIC;
    }
    return NO_ERROR;
}

// resolve the interrupt an interrupt link device (PNP0C0F) is currently routed to
static status_t link_resolve(uacpi_namespace_node *link, unsigned int index,
                             struct acpi_pci_irq *out) {
    for (int attempt = 0; attempt < 2; attempt++) {
        struct link_irq_ctx ctx = { .wanted_index = index };
        uacpi_status st = uacpi_for_each_device_resource(link, "_CRS", link_irq_callback, &ctx);
        if (st != UACPI_STATUS_OK) {
            const char *path = uacpi_namespace_node_generate_absolute_path(link);
            printf("ACPI PCI: failed to evaluate _CRS of interrupt link %s: %s\n",
                   path ? path : "?", uacpi_status_to_string(st));
            uacpi_free_absolute_path(path);
            return ERR_GENERIC;
        }
        if (ctx.found && ctx.irq.gsi != 0) {
            *out = ctx.irq;
            return NO_ERROR;
        }

        // not routed to anything, try to enable it once
        if (attempt == 0) {
            const char *path = uacpi_namespace_node_generate_absolute_path(link);
            printf("ACPI PCI: interrupt link %s is disabled, trying to enable it\n",
                   path ? path : "?");
            uacpi_free_absolute_path(path);
            if (link_enable(link) != NO_ERROR) {
                break;
            }
        }
    }

    return ERR_NOT_FOUND;
}

// look up (dev, pin) in a routing table, resolving links. returns ERR_NOT_FOUND if not listed.
static status_t prt_lookup(const uacpi_pci_routing_table *prt, unsigned int dev, unsigned int pin,
                           struct acpi_pci_irq *out) {
    for (size_t i = 0; i < prt->num_entries; i++) {
        const uacpi_pci_routing_table_entry *entry = &prt->entries[i];

        // address is device << 16 | function, function is always 0xffff (all)
        if ((entry->address >> 16) != dev) {
            continue;
        }
        if (entry->pin != pin - 1) {
            continue;
        }

        if (entry->source == NULL) {
            // hardwired global system interrupt. pci interrupts are level/active low.
            out->gsi = entry->index;
            out->level_triggered = true;
            out->active_low = true;
            return NO_ERROR;
        }

        return link_resolve(entry->source, entry->index, out);
    }

    return ERR_NOT_FOUND;
}

// find the child device node of parent whose _ADR names dev/fn
struct adr_lookup {
    uint64_t adr;
    uacpi_namespace_node *found;
};

static uacpi_iteration_decision adr_lookup_callback(void *user, uacpi_namespace_node *node,
                                                    uacpi_u32 depth) {
    struct adr_lookup *lookup = user;

    uacpi_u64 adr;
    if (uacpi_eval_adr(node, &adr) != UACPI_STATUS_OK) {
        return UACPI_ITERATION_DECISION_CONTINUE;
    }
    // _ADR may name a specific function or all of them
    if (adr == lookup->adr || adr == (lookup->adr | 0xffff)) {
        lookup->found = node;
        return UACPI_ITERATION_DECISION_BREAK;
    }
    return UACPI_ITERATION_DECISION_CONTINUE;
}

static uacpi_namespace_node *find_child_by_adr(uacpi_namespace_node *parent, unsigned int dev,
                                               unsigned int fn) {
    struct adr_lookup lookup = { .adr = ((uint64_t)dev << 16) | fn, .found = NULL };
    uacpi_namespace_for_each_child(parent, adr_lookup_callback, NULL, UACPI_OBJECT_DEVICE_BIT, 1,
                                   &lookup);
    return lookup.found;
}

status_t acpi_pci_root_route_intx(struct acpi_pci_root *root, const struct pci_intx_path_entry *path,
                                  size_t path_len, unsigned int pin, struct acpi_pci_irq *out) {
    LTRACEF("root %04x:%02x path len %zu pin %u\n", root->segment, root->bus_start, path_len, pin);

    if (pin < 1 || pin > 4 || path_len == 0 || !path || !out) {
        return ERR_INVALID_ARGS;
    }
    for (size_t i = 0; i < path_len; i++) {
        if (path[i].dev >= 32 || path[i].fn >= 8) {
            return ERR_INVALID_ARGS;
        }
    }

    status_t err = acpi_pci_root_load_prt(root);
    if (err != NO_ERROR) {
        return err;
    }

    // walk down from the root to find the device node that owns each bus on the path:
    // bus_node[i] is the node whose _PRT governs the bus path[i] sits on. once a bridge has no
    // node of its own nothing below it can either.
    uacpi_namespace_node *bus_node[path_len];
    bus_node[0] = root->node;
    for (size_t i = 1; i < path_len; i++) {
        bus_node[i] = bus_node[i - 1] ? find_child_by_adr(bus_node[i - 1], path[i - 1].dev, path[i - 1].fn)
                                      : NULL;
    }

    // then walk back up: a bridge's own _PRT beats swizzling (ACPI 6.x 6.2.13), and the root's
    // _PRT is the last resort
    unsigned int dev = path[path_len - 1].dev;
    for (size_t i = path_len; i-- > 0;) {
        if (i == 0) {
            return prt_lookup(root->prt, dev, pin, out);
        }

        if (bus_node[i]) {
            uacpi_pci_routing_table *prt = NULL;
            uacpi_status st = uacpi_get_pci_routing_table(bus_node[i], &prt);
            if (st == UACPI_STATUS_OK) {
                err = prt_lookup(prt, dev, pin, out);
                uacpi_free_pci_routing_table(prt);
                if (err == NO_ERROR) {
                    return NO_ERROR;
                }
                LTRACEF("bridge level %zu has a _PRT but no entry for dev %u pin %u\n", i, dev, pin);
            }
        }

        // no help at this level, swizzle up through the bridge: each PCI-PCI bridge rotates
        // INTA-D by the device number below it (PCI-PCI bridge spec 9.1)
        pin = ((pin - 1 + dev) % 4) + 1;
        dev = path[i - 1].dev;
    }

    return ERR_NOT_FOUND;
}

#endif // WITH_DEV_BUS_PCI
