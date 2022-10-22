/*
 * Copyright (c) 2009 Corey Tabaka
 * Copyright (c) 2020 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include "ecam.h"

#include <lk/debug.h>
#include <lk/err.h>
#include <stdlib.h>
#include <string.h>
#include <kernel/thread.h>
#include <kernel/spinlock.h>
#include <dev/bus/pci.h>
#include <lk/trace.h>

#if WITH_KERNEL_VM
#include <kernel/vm.h>
#endif

#include "../pci_priv.h"

#define LOCAL_TRACE 0

pci_ecam::pci_ecam(paddr_t base, uint16_t segment, uint8_t start_bus, uint8_t end_bus) :
    base_(base), segment_(segment), start_bus_(start_bus), end_bus_(end_bus) {}

pci_ecam::~pci_ecam() {
    LTRACE_ENTRY;
#if WITH_KERNEL_VM
    if (ecam_ptr_) {
        vmm_free_region(vmm_get_kernel_aspace(), (vaddr_t)ecam_ptr_);
    }
#endif
}

pci_ecam *pci_ecam::detect(paddr_t base, uint16_t segment, uint8_t start_bus, uint8_t end_bus) {
    LTRACEF("base %#lx, segment %hu, bus [%hhu...%hhu]\n", base, segment, start_bus, end_bus);

    // we only support a limited configuration at the moment
    if (segment != 0 || start_bus != 0) {
        return nullptr;
    }

    auto ecam = new pci_ecam(base, segment, start_bus, end_bus);

    // initialize the object, which may fail
    status_t err = ecam->initialize();
    if (err != NO_ERROR) {
        delete ecam;
        return nullptr;
    }

    return ecam;
}

status_t pci_ecam::initialize() {
    // compute the aperture size of this
    size_t size = ((size_t)end_bus_ - (size_t)start_bus_ + 1) << 20; // each bus occupies 20 bits of address space
    LTRACEF("aperture size %#zx\n", size);


#if WITH_KERNEL_VM
    // try to map the aperture
    // ask for 4MB aligned regions (log2 22) to help with the mmu on most architectures
    //status_t vmm_alloc_physical(vmm_aspace_t *aspace, const char *name, size_t size, void **ptr, uint8_t align_log2, paddr_t paddr, uint vmm_flags, uint arch_mmu_flags)
    status_t err = vmm_alloc_physical(vmm_get_kernel_aspace(), "pci_ecam", size, (void **)&ecam_ptr_, 22, base_, 0, ARCH_MMU_FLAG_UNCACHED_DEVICE);
    LTRACEF("vmm_alloc_physical returns %d, ptr %p\n", err, ecam_ptr_);

    if (err != NO_ERROR) {
        ecam_ptr_ = nullptr;
        return err;
    }
#else
    // no vm, so can directly access the aperture
    ecam_ptr_ = (uint8_t *)base_;
#endif

    set_last_bus(end_bus_);

    return NO_ERROR;
}

// compute the offset into the ecam given the location and register offset
inline size_t location_to_offset(const pci_location_t state, uint32_t reg) {
    //
    // | 27 - 20 | 19 - 15 | 14 - 12     |  11 - 8          | 7 - 2       | 1 - 0       |
    // | Bus Nr  | Dev Nr  | Function Nr | Ext. Register Nr | Register Nr | Byte Enable |

    // TODO: clamp or assert on invalid offset
    size_t offset = (size_t)state.bus << 20;
    offset += (size_t)state.dev << 15;
    offset += (size_t)state.fn << 12;
    offset += reg;
    return offset;
}

// templatized routines to access the pci config space using a specific type
template <typename T>
inline int read_config(const pci_location_t state, uint32_t reg, T *value, const uint8_t *ecam_ptr) {
    auto off = location_to_offset(state, reg);

    *value = *reinterpret_cast<const volatile T *>(&ecam_ptr[off]);

    return NO_ERROR;
}

template <typename T>
inline int write_config(const pci_location_t state, uint32_t reg, T value, uint8_t *ecam_ptr) {
    auto off = location_to_offset(state, reg);

    *reinterpret_cast<volatile T *>(&ecam_ptr[off]) = value;

    return NO_ERROR;
}

int pci_ecam::read_config_byte(const pci_location_t state, uint32_t reg, uint8_t *value) {
    LTRACEF_LEVEL(2, "state bus %#hhx dev %#hhx %#hhx reg %#x\n", state.bus, state.dev, state.fn, reg);
    return read_config(state, reg, value, ecam_ptr_);
}

int pci_ecam::read_config_half(const pci_location_t state, uint32_t reg, uint16_t *value) {
    LTRACEF_LEVEL(2, "state bus %#hhx dev %#hhx %#hhx reg %#x\n", state.bus, state.dev, state.fn, reg);
    return read_config(state, reg, value, ecam_ptr_);
}

int pci_ecam::read_config_word(const pci_location_t state, uint32_t reg, uint32_t *value) {
    LTRACEF_LEVEL(2, "state bus %#hhx dev %#hhx %#hhx reg %#x\n", state.bus, state.dev, state.fn, reg);
    return read_config(state, reg, value, ecam_ptr_);
}

int pci_ecam::write_config_byte(const pci_location_t state, uint32_t reg, uint8_t value) {
    LTRACEF_LEVEL(2, "state bus %#hhx dev %#hhx %#hhx reg %#x\n", state.bus, state.dev, state.fn, reg);
    return write_config(state, reg, value, ecam_ptr_);
}

int pci_ecam::write_config_half(const pci_location_t state, uint32_t reg, uint16_t value) {
    LTRACEF_LEVEL(2, "state bus %#hhx dev %#hhx %#hhx reg %#x\n", state.bus, state.dev, state.fn, reg);
    return write_config(state, reg, value, ecam_ptr_);
}

int pci_ecam::write_config_word(const pci_location_t state, uint32_t reg, uint32_t value) {
    LTRACEF_LEVEL(2, "state bus %#hhx dev %#hhx %#hhx reg %#x\n", state.bus, state.dev, state.fn, reg);
    return write_config(state, reg, value, ecam_ptr_);
}
