// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

/*
 * xHCI bring-up TODOs (next steps):
 * 1) Add a compact port-status debug command to dump PORTSC for all ports
 *    (CCS/PED/PLS/PortSpeed/PR/PP) so attach/detach behavior can be observed.
 * 2) Implement minimal host controller state transitions:
 *    - halt/run sequencing via USBCMD/USBSTS
 *    - host controller reset and ready polling
 *    - preserve/restore basic command bits safely.
 * 3) Parse and expose operational/runtime register layout from capability
 *    offsets (CAPLENGTH/DBOFF/RTSOFF) with typed access helpers.
 * 4) Build an initialization skeleton for command processing:
 *    - allocate DCBAA
 *    - allocate command ring + enqueue/dequeue management
 *    - program CRCR and basic interrupter/event ring structures.
 * 5) Add a lightweight event-ring consumer for command completion and basic
 *    error reporting (no USB class stack yet).
 * 6) Add optional MSI/MSI-X hookup for xHCI with fallback to legacy IRQ,
 *    with IRQ debug counters and interrupt cause tracing.
 * 7) Extend extended-cap decoding to show protocol details in a structured
 *    controller summary object that can be reused by future USB stack code.
 */

#include <arch/atomic.h>
#include <dev/bus/pci.h>
#include <kernel/vm.h>
#include <lk/bits.h>
#include <lk/console_cmd.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/init.h>
#include <lk/list.h>
#include <lk/reg.h>
#include <lk/trace.h>
#include <stdlib.h>
#include <string.h>

#define LOCAL_TRACE 0

namespace {

constexpr uint8_t XHCI_PCI_SUBCLASS_USB = 0x03;
constexpr uint8_t XHCI_PCI_PROGIF_XHCI = 0x30;

enum class xhci_cap_reg : uint32_t {
    CAPLENGTH = 0x00,
    HCIVERSION = 0x02,
    HCSPARAMS1 = 0x04,
    HCSPARAMS2 = 0x08,
    HCSPARAMS3 = 0x0c,
    HCCPARAMS1 = 0x10,
    DBOFF = 0x14,
    RTSOFF = 0x18,
    HCCPARAMS2 = 0x1c,
};

enum class xhci_hccparams1_bit : uint32_t {
    AC64 = 0,
    BNC = 1,
    CSZ = 2,
    PPC = 3,
    LTC = 6,
    LHRC = 7,
    NSS = 8,
};

static bool hccparams1_has(uint32_t val, xhci_hccparams1_bit bit) {
    return val & (1u << static_cast<uint32_t>(bit));
}

class xhci_pci_controller;
static list_node controllers = LIST_INITIAL_VALUE(controllers);
static size_t controller_count;

class xhci_pci_controller {
  public:
    xhci_pci_controller() = default;
    ~xhci_pci_controller() = default;

    status_t init(pci_location_t loc);

    uint unit() const { return unit_; }
    pci_location_t loc() const { return loc_; }
    size_t mmio_len() const { return mmio_len_; }
    list_node *list_node_ptr() { return &node_; }
    static xhci_pci_controller *from_list_node(list_node *node) {
        return containerof(node, xhci_pci_controller, node_);
    }

    void dump() const {
        char str[14];
        printf("xhci%u at %s:\n", unit_, pci_loc_string(loc_, str));
        enumerate_capabilities();
    }

  private:
    static constexpr uint32_t reg_offset(xhci_cap_reg reg) {
        return static_cast<uint32_t>(reg);
    }

    uint8_t read8(xhci_cap_reg reg) const {
        return mmio_read8(mmio_base_ + reg_offset(reg));
    }

    uint16_t read16(xhci_cap_reg reg) const {
        return mmio_read16(reinterpret_cast<volatile uint16_t *>(mmio_base_ + reg_offset(reg)));
    }

    uint32_t read32(xhci_cap_reg reg) const {
        return mmio_read32(reinterpret_cast<volatile uint32_t *>(mmio_base_ + reg_offset(reg)));
    }

    uint32_t read32(uint32_t offset) const {
        return mmio_read32(reinterpret_cast<volatile uint32_t *>(mmio_base_ + offset));
    }

    void enumerate_capabilities() const;
    void enumerate_extended_capabilities(uint32_t hccparams1) const;

    static int global_count_;

    uint unit_ = 0;
    pci_location_t loc_ = {};
    volatile uint8_t *mmio_base_ = nullptr;
    size_t mmio_len_ = 0;
    list_node node_ = LIST_INITIAL_CLEARED_VALUE;
};

int xhci_pci_controller::global_count_ = 0;

status_t xhci_pci_controller::init(pci_location_t loc) {
    loc_ = loc;
    char str[32];

    LTRACEF("pci location %s\n", pci_loc_string(loc_, str));

    pci_bar_t bars[6];
    status_t err = pci_bus_mgr_read_bars(loc_, bars);
    if (err != NO_ERROR) {
        return err;
    }

    if (LOCAL_TRACE) {
        pci_dump_bars(bars, 6);
    }

    if (!bars[0].valid || bars[0].io || bars[0].addr == 0 || bars[0].size == 0) {
        return ERR_NOT_FOUND;
    }

    mmio_len_ = PAGE_ALIGN(bars[0].size);
    if (mmio_len_ < PAGE_SIZE) {
        mmio_len_ = PAGE_SIZE;
    }

    unit_ = atomic_add(&global_count_, 1);
    snprintf(str, sizeof(str), "xhci%u bar0", unit_);

    void *regs = nullptr;
    err = vmm_alloc_physical(vmm_get_kernel_aspace(), str, mmio_len_, &regs, 0,
                             bars[0].addr, /* vmm_flags */ 0, ARCH_MMU_FLAG_UNCACHED_DEVICE);
    if (err != NO_ERROR) {
        return err;
    }
    mmio_base_ = reinterpret_cast<volatile uint8_t *>(regs);

    pci_bus_mgr_enable_device(loc_);

    pci_config_t config = {};
    err = pci_read_config(loc_, &config);
    if (err == NO_ERROR) {
        dprintf(INFO, "xhci%u: %04x:%04x rev %#x at %s\n", unit_, config.vendor_id,
                config.device_id, config.revision_id_0, pci_loc_string(loc_, str));
    } else {
        dprintf(INFO, "xhci%u: found at %s\n", unit_, pci_loc_string(loc_, str));
    }

    enumerate_capabilities();

    return NO_ERROR;
}

void xhci_pci_controller::enumerate_extended_capabilities(uint32_t hccparams1) const {
    printf("xhci%u: enumerating extended capabilities, hccparams1 %#x\n", unit_, hccparams1);
    uint32_t ext_cap = BITS_SHIFT(hccparams1, 31, 16) * 4;
    printf("xhci%u: first ext cap offset %#x\n", unit_, ext_cap);
    if (ext_cap == 0) {
        return;
    }
    if ((ext_cap + 4) > mmio_len_) {
        return;
    }

    dprintf(INFO, "xhci%u: ext caps at %#x\n", unit_, ext_cap);
    for (size_t i = 0; i < 64; ++i) {
        if ((ext_cap + 4) > mmio_len_) {
            break;
        }

        uint32_t cap = read32(ext_cap);
        uint8_t cap_id = BITS_SHIFT(cap, 7, 0);
        uint8_t next = BITS_SHIFT(cap, 15, 8);
        uint8_t rev_major = BITS_SHIFT(cap, 23, 20);
        uint8_t rev_minor = BITS_SHIFT(cap, 19, 16);

        dprintf(INFO, "xhci%u: ext cap[%zu] off=%#x id=%#x rev=%u.%u\n", unit_, i,
                ext_cap, cap_id, rev_major, rev_minor);

        if (cap_id == 0x2) {
            if ((ext_cap + 0x10) <= mmio_len_) {
                uint32_t name = read32(ext_cap + 0x4);
                uint32_t ports = read32(ext_cap + 0x8);
                uint32_t slot = read32(ext_cap + 0xc);

                char name_str[5] = {
                    static_cast<char>(BITS_SHIFT(name, 7, 0)),
                    static_cast<char>(BITS_SHIFT(name, 15, 8)),
                    static_cast<char>(BITS_SHIFT(name, 23, 16)),
                    static_cast<char>(BITS_SHIFT(name, 31, 24)),
                    '\0',
                };

                uint8_t proto_major = BITS_SHIFT(cap, 31, 24);
                uint8_t proto_minor = BITS_SHIFT(cap, 23, 16);
                uint8_t compat_port_off = BITS_SHIFT(ports, 7, 0);
                uint8_t compat_port_count = BITS_SHIFT(ports, 15, 8);
                uint8_t psic = BITS_SHIFT(ports, 31, 28);
                uint8_t slot_type = BITS_SHIFT(slot, 4, 0);

                dprintf(INFO,
                        "xhci%u:   protocol %s rev %u.%u ports [%u..%u] psic %u slot_type %u\n",
                        unit_, name_str, proto_major, proto_minor, compat_port_off,
                        static_cast<unsigned>(compat_port_off + compat_port_count - 1),
                        psic, slot_type);

                if (psic > 0) {
                    size_t psi_base = ext_cap + 0x10;
                    size_t psi_end = psi_base + static_cast<size_t>(psic) * sizeof(uint32_t);
                    if (psi_end <= mmio_len_) {
                        for (uint8_t psi_idx = 0; psi_idx < psic; ++psi_idx) {
                            uint32_t psi = read32(psi_base + static_cast<size_t>(psi_idx) * sizeof(uint32_t));
                            uint8_t psiv = BITS_SHIFT(psi, 3, 0);
                            uint8_t psik = BITS_SHIFT(psi, 5, 4);
                            uint16_t psie = BITS_SHIFT(psi, 15, 6);
                            uint8_t plt = BITS_SHIFT(psi, 17, 16);
                            uint8_t pfd = BITS_SHIFT(psi, 18, 18);

                            dprintf(INFO,
                                    "xhci%u:     psi[%u] psiv %u psik %u psie %u plt %u pfd %u raw %#x\n",
                                    unit_, psi_idx, psiv, psik, psie, plt, pfd, psi);
                        }
                    } else {
                        dprintf(INFO,
                                "xhci%u:     psi table truncated (psic %u, base %#zx, len %#zx)\n",
                                unit_, psic, psi_base, mmio_len_);
                    }
                }
            } else {
                dprintf(INFO, "xhci%u:   protocol cap truncated at off %#x\n", unit_, ext_cap);
            }
        }

        if (next == 0) {
            break;
        }

        uint32_t next_ext_cap = ext_cap + static_cast<uint32_t>(next) * 4;
        if (next_ext_cap <= ext_cap || (next_ext_cap + 4) > mmio_len_) {
            dprintf(INFO, "xhci%u: invalid ext cap next %#x from off %#x\n", unit_,
                    next_ext_cap, ext_cap);
            break;
        }

        ext_cap = next_ext_cap;
    }
}

void xhci_pci_controller::enumerate_capabilities() const {
    uint32_t capreg = read32(xhci_cap_reg::CAPLENGTH);
    uint8_t cap_length = BITS_SHIFT(capreg, 7, 0);
    uint16_t hci_version = BITS_SHIFT(capreg, 31, 16);
    uint32_t hcsparams1 = read32(xhci_cap_reg::HCSPARAMS1);
    uint32_t hcsparams2 = read32(xhci_cap_reg::HCSPARAMS2);
    uint32_t hcsparams3 = read32(xhci_cap_reg::HCSPARAMS3);
    uint32_t hccparams1 = read32(xhci_cap_reg::HCCPARAMS1);
    uint32_t dboff = read32(xhci_cap_reg::DBOFF);
    uint32_t rtsoff = read32(xhci_cap_reg::RTSOFF);
    uint32_t hccparams2 = 0;

    if (cap_length >= (reg_offset(xhci_cap_reg::HCCPARAMS2) + 4)) {
        hccparams2 = read32(xhci_cap_reg::HCCPARAMS2);
    }

    uint32_t max_slots = BITS_SHIFT(hcsparams1, 7, 0);
    uint32_t max_intrs = BITS_SHIFT(hcsparams1, 18, 8);
    uint32_t max_ports = BITS_SHIFT(hcsparams1, 31, 24);

    uint32_t max_scratchpad =
        (BITS_SHIFT(hcsparams2, 31, 27) << 5) | BITS_SHIFT(hcsparams2, 25, 21);

    dprintf(INFO,
            "xhci%u: caplen %#x ver %u.%u ports %u slots %u intrs %u scratchpad %u\n",
            unit_, cap_length, hci_version >> 8, hci_version & 0xff, max_ports,
            max_slots, max_intrs, max_scratchpad);

    dprintf(INFO,
            "xhci%u: dboff %#x rtsoff %#x ac64 %u bnc %u csz %u ppc %u lhrc %u ltc %u nss %u\n",
            unit_, dboff, rtsoff,
            hccparams1_has(hccparams1, xhci_hccparams1_bit::AC64),
            hccparams1_has(hccparams1, xhci_hccparams1_bit::BNC),
            hccparams1_has(hccparams1, xhci_hccparams1_bit::CSZ),
            hccparams1_has(hccparams1, xhci_hccparams1_bit::PPC),
            hccparams1_has(hccparams1, xhci_hccparams1_bit::LHRC),
            hccparams1_has(hccparams1, xhci_hccparams1_bit::LTC),
            hccparams1_has(hccparams1, xhci_hccparams1_bit::NSS));

    if (hccparams2 != 0) {
        dprintf(INFO, "xhci%u: hccparams2 %#x\n", unit_, hccparams2);
    }

    if (LOCAL_TRACE) {
        dprintf(INFO, "xhci%u: hcsparams3 %#x hccparams1 %#x\n", unit_, hcsparams3,
                hccparams1);
    }

    enumerate_extended_capabilities(hccparams1);
}

void xhci_pci_init(uint level) {
    LTRACE_ENTRY;

    for (size_t i = 0;; ++i) {
        pci_location_t loc;
        status_t err = pci_bus_mgr_find_device_by_class(&loc, PCI_CLASS_SERIAL_BUS,
                                                        XHCI_PCI_SUBCLASS_USB,
                                                        XHCI_PCI_PROGIF_XHCI, i);
        if (err != NO_ERROR) {
            break;
        }

        auto *xhci = new xhci_pci_controller();
        err = xhci->init(loc);
        if (err != NO_ERROR) {
            char str[14];
            printf("xhci: device at %s failed to initialize (%d)\n", pci_loc_string(loc, str), err);
            delete xhci;
            continue;
        }

        list_add_tail(&controllers, xhci->list_node_ptr());
        controller_count++;
    }
}
LK_INIT_HOOK(xhci_pci, &xhci_pci_init, LK_INIT_LEVEL_PLATFORM + 1);

int xhci_cmd(int argc, const console_cmd_args *argv) {
    if (argc < 2) {
usage:
        printf("xhci commands:\n");
        printf("%s list\n", argv[0].str);
        printf("%s dump all\n", argv[0].str);
        printf("%s dump <unit>\n", argv[0].str);
        return -1;
    }

    if (!strcmp(argv[1].str, "list")) {
        printf("xhci: %zu controller(s)\n", controller_count);
        list_node *node;
        list_for_every(&controllers, node) {
            auto *ctrl = xhci_pci_controller::from_list_node(node);
            char str[14];
            pci_config_t cfg = {};
            status_t err = pci_read_config(ctrl->loc(), &cfg);
            if (err == NO_ERROR) {
                printf("  xhci%u %s %04x:%04x rev %#x mmio %zu\n", ctrl->unit(),
                       pci_loc_string(ctrl->loc(), str), cfg.vendor_id, cfg.device_id,
                       cfg.revision_id_0, ctrl->mmio_len());
            } else {
                printf("  xhci%u %s mmio %zu\n", ctrl->unit(),
                       pci_loc_string(ctrl->loc(), str), ctrl->mmio_len());
            }
        }
        return 0;
    }

    if (!strcmp(argv[1].str, "dump")) {
        if (argc < 3 || !strcmp(argv[2].str, "all")) {
            list_node *node;
            list_for_every(&controllers, node) {
                auto *ctrl = xhci_pci_controller::from_list_node(node);
                ctrl->dump();
            }
            return 0;
        }

        uint unit = atoui(argv[2].str);
        list_node *node;
        list_for_every(&controllers, node) {
            auto *ctrl = xhci_pci_controller::from_list_node(node);
            if (ctrl->unit() == unit) {
                ctrl->dump();
                return 0;
            }
        }

        printf("xhci: controller xhci%u not found\n", unit);
        return ERR_NOT_FOUND;
    }

    goto usage;
}

STATIC_COMMAND_START
STATIC_COMMAND("xhci", "xhci debug commands", &xhci_cmd)
STATIC_COMMAND_END(xhci);

} // namespace
