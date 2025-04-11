// Copyright 2020 The Fuchsia Authors
// Copyright 2021 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#include <lib/acpi_lite.h>

#include <inttypes.h>
#include <string.h>
#include <lk/compiler.h>
#include <lk/cpp.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <kernel/vm.h>

// uses the vm to map in ACPI tables as they are found
static_assert(WITH_KERNEL_VM);

#define LOCAL_TRACE 0

// global state of the acpi lite library
struct acpi_lite_state {
  const acpi_rsdp* rsdp;
  paddr_t rsdp_pa;
  const acpi_rsdt_xsdt* sdt;
  paddr_t sdt_pa;
  bool xsdt;          // are the pointers in the SDT 64 or 32bit?

  size_t num_tables;  // number of top level tables
  const void **tables; // array of pointers to detected tables
} acpi;

// map a region around a physical address
static void *map_region(paddr_t pa, size_t len, const char *name) {
  const auto pa_page_aligned = ROUNDDOWN(pa, PAGE_SIZE);
  const size_t align_offset = pa - pa_page_aligned;
  size_t map_len = ROUNDUP(len + align_offset, PAGE_SIZE);

  uint perms = ARCH_MMU_FLAG_PERM_RO;
  if (arch_mmu_supports_nx_mappings()) {
    perms |= ARCH_MMU_FLAG_PERM_NO_EXECUTE;
  }

  void *ptr;
  status_t err = vmm_alloc_physical(vmm_get_kernel_aspace(), name, map_len,
                                    &ptr, 0, pa_page_aligned, 0, perms);
  if (err < 0) {
    return nullptr;
  }

  return (void *)((uintptr_t)ptr + align_offset);
}

static uint8_t acpi_checksum(const void* _buf, size_t len) {
  const uint8_t* buf = static_cast<const uint8_t*>(_buf);

  uint8_t c = 0;
  for (size_t i = 0; i < len; i++) {
    c += buf[i];
  }

  return c;
}

static bool validate_rsdp(const acpi_rsdp* rsdp, bool debug_output = true) {
  // check the signature
  if (memcmp(ACPI_RSDP_SIG, rsdp->sig, 8)) {
    // Generates a huge pile of info as it scans for RSDP
    if (debug_output && LOCAL_TRACE) {
      LTRACEF("acpi rsdp signature failed:\n");
      hexdump8(rsdp->sig, 8);
    }
    return false;
  }

  // validate the v1 checksum on the first 20 bytes of the table
  uint8_t c = acpi_checksum(rsdp, 20);
  if (c) {
    LTRACEF("v1 checksum failed\n");
    return false;
  }

  // is it v2?
  LTRACEF("rsdp version %u\n", rsdp->revision);
  if (rsdp->revision >= 2) {
    LTRACEF("rsdp length %u\n", rsdp->length);
    if (rsdp->length < 36 || rsdp->length > 4096) {
      // keep the table length within reason
      return false;
    }

    c = acpi_checksum(rsdp, rsdp->length);
    if (c) {
      LTRACEF("full checksum failed\n");
      return false;
    }
  }

  // seems okay
  return true;
}

// search the bios region on a PC for the Root System Description Pointer (RSDP)
static paddr_t find_rsdp_pc() {
  LTRACE_ENTRY;

  const paddr_t range_start = 0xe0000;
  const paddr_t range_end = 0x100000;
  const size_t len = range_end - range_start;

  // map all of the scannable area, 0xe0000...1MB
  const uint8_t *bios_ptr;
  status_t err = vmm_alloc_physical(vmm_get_kernel_aspace(), "acpi rsdp bios area", len,
                                    (void **)&bios_ptr, 0, range_start, 0, ARCH_MMU_FLAG_PERM_RO);
  if (err < 0) {
    return 0;
  }
  LTRACEF("bios area mapping at %p\n", bios_ptr);

  // free the region when we exit
  auto ac = lk::make_auto_call([bios_ptr]() {
      vmm_free_region(vmm_get_kernel_aspace(), (vaddr_t)bios_ptr);
  });

  // search for it in the BIOS EBDA area (0xe0000..0xfffff) on 16 byte boundaries
  for (size_t i = 0; i < len; i += 16) {
    const auto rsdp = reinterpret_cast<const acpi_rsdp*>(bios_ptr + i);

    if (validate_rsdp(rsdp, false)) {
      LTRACEF("found rsdp at vaddr %p, paddr %#lx\n", bios_ptr + i, range_start + i);
      return range_start + i;
    }
  }

  return 0;
}

static bool validate_sdt(const acpi_rsdt_xsdt* sdt, size_t* num_tables, bool* xsdt) {
  LTRACEF("pointer %p\n", sdt);

  // bad pointer
  if (!sdt) {
    LTRACEF("failing due to null pointer\n");
    return false;
  }

  // check the signature and see if it's a rsdt or xsdt
  if (!memcmp(sdt->header.sig, "XSDT", 4)) {
    LTRACEF("found XSDT\n");
    *xsdt = true;
  } else if (!memcmp(sdt->header.sig, "RSDT", 4)) {
    LTRACEF("found RSDT\n");
    *xsdt = false;
  } else {
    LTRACEF("did not find XSDT or RSDT\n");
    return false;
  }

  // is the length sane?
  if (sdt->header.length < 36 || sdt->header.length > 4096) {
    LTRACEF("bad length %u\n", sdt->header.length);
    return false;
  }

  // is it a revision we understand?
  if (sdt->header.revision != 1) {
    LTRACEF("revision we do not handle %u\n", sdt->header.revision);
    return false;
  }

  // checksum the entire table
  uint8_t c = acpi_checksum(sdt, sdt->header.length);
  if (c) {
    LTRACEF("failed checksum\n");
    return false;
  }

  // compute the number of pointers to tables we have
  *num_tables = (sdt->header.length - 36u) / (*xsdt ? 8u : 4u);

  // looks okay
  return true;
}

static paddr_t acpi_get_table_pa_at_index(size_t index) {
  if (index >= acpi.num_tables) {
    return 0;
  }

  paddr_t pa;
  if (acpi.xsdt) {
    pa = acpi.sdt->addr64[index];
  } else {
    pa = acpi.sdt->addr32[index];
  }
  LTRACEF("index %zu, pa %#lx\n", index, pa);

  return pa;
}

static const acpi_sdt_header* acpi_get_table_at_index(size_t index) {
  if (index >= acpi.num_tables) {
    return nullptr;
  }

  return static_cast<const acpi_sdt_header *>(acpi.tables[index]);
}

const acpi_sdt_header* acpi_get_table_by_sig(const char* sig) {
  // walk the list of tables
  for (size_t i = 0; i < acpi.num_tables; i++) {
    const auto header = acpi_get_table_at_index(i);
    if (!header) {
      continue;
    }

    if (!memcmp(sig, header->sig, 4)) {
      // checksum should already have been validated when the table was loaded
      return header;
    }
  }

  return nullptr;
}

static status_t initialize_table(size_t i) {
  char name[64];
  snprintf(name, sizeof(name), "acpi table %zu", i);

  const size_t table_initial_len = PAGE_SIZE; // enough to read the header
  auto pa = acpi_get_table_pa_at_index(i);

  const acpi_sdt_header *header = (const acpi_sdt_header *)map_region(pa, table_initial_len, name);
  if (!header) {
    dprintf(INFO, "ACPI LITE: failed to map table %zu address %#" PRIxPTR "\n", i, pa);
    return ERR_NOT_FOUND;
  }

  // cleanup the mapping that maps just the first page when we exit
  auto cleanup_header_mapping = lk::make_auto_call([header]() {
      vmm_free_region(vmm_get_kernel_aspace(), ROUNDDOWN((vaddr_t)header, PAGE_SIZE));
  });

  // check the header and determine the real size
  if (header->length > 1024*1024) {
    // probably bogus?
    dprintf(INFO, "ACPI LITE: table %zu has length %u, too large\n", i, header->length);
    return ERR_NOT_FOUND;
  }
  if (header->length < sizeof(*header)) {
    return ERR_NOT_FOUND;
  }

  // try to map the real table
  char sig[5] = {};
  sig[0] = header->sig[0];
  sig[1] = header->sig[1];
  sig[2] = header->sig[2];
  sig[3] = header->sig[3];
  snprintf(name, sizeof(name), "acpi table %s", sig);
  acpi.tables[i] = map_region(pa, header->length, name);
  if (!acpi.tables[i]) {
    dprintf(INFO, "ACPI LITE: failed to map table %zu address %#" PRIxPTR "\n", i, pa);
    return ERR_NOT_FOUND;
  }

  LTRACEF("table %zu (%s) mapped at %p\n", i, sig, acpi.tables[i]);

  // ODO compute checksum on table?
  header = (const acpi_sdt_header *)acpi.tables[i];
  uint8_t c = acpi_checksum(header, header->length);
  if (c != 0) {
    dprintf(INFO, "ACPI LITE: table %zu (%s) fails checksum\n", i, sig);
    acpi.tables[i] = nullptr;
    return ERR_NOT_FOUND;
  }

  return NO_ERROR;
}

status_t acpi_lite_init(paddr_t rsdp_pa) {
  LTRACEF("passed in rsdp %#" PRIxPTR "\n", rsdp_pa);

  // see if the rsdp pointer is valid
  if (rsdp_pa == 0) {
    // search around for it in a platform-specific way
#if PLATFORM_PC
    rsdp_pa = find_rsdp_pc();
    if (rsdp_pa == 0) {
      dprintf(INFO, "ACPI LITE: couldn't find ACPI RSDP in BIOS area\n");
    }
#endif

    if (rsdp_pa == 0) {
      return ERR_NOT_FOUND;
    }
  }

  const size_t rsdp_area_len = 0x1000; // 4K should cover it. TODO: see if it's specced
  const void * const rsdp_ptr = map_region(rsdp_pa, rsdp_area_len, "acpi rsdp area");
  if (!rsdp_ptr) {
    dprintf(INFO, "ACPI LITE: failed to map RSDP address %#" PRIxPTR " to virtual\n", rsdp_pa);
    return ERR_NOT_FOUND;
  }
  LTRACEF("rsdp mapped at %p\n", rsdp_ptr);

  // free the region if we abort
  auto cleanup_rsdp_mapping = lk::make_auto_call([rsdp_ptr]() {
      vmm_free_region(vmm_get_kernel_aspace(), ROUNDDOWN((vaddr_t)rsdp_ptr, PAGE_SIZE));
      acpi.rsdp_pa = 0;
      acpi.rsdp = nullptr;
  });

  // see if the RSDP is there
  acpi.rsdp = static_cast<const acpi_rsdp*>(rsdp_ptr);
  if (!validate_rsdp(acpi.rsdp)) {
    dprintf(INFO, "ACPI LITE: RSDP structure does not check out\n");
    return ERR_NOT_FOUND;
  }
  acpi.rsdp_pa = rsdp_pa;

  dprintf(SPEW, "ACPI LITE: RSDP checks out, found at %#lx, revision %u\n",
      acpi.rsdp_pa, acpi.rsdp->revision);

  // find the pointer to either the RSDT or XSDT
  acpi.sdt = nullptr;
  if (acpi.rsdp->revision < 2) {
    // v1 RSDP, pointing at a RSDT
    LTRACEF("v1 RSDP, using 32 bit RSDT address %#x\n", acpi.rsdp->rsdt_address);
    acpi.sdt_pa = acpi.rsdp->rsdt_address;
  } else {
    // v2+ RSDP, pointing at a XSDT
    LTRACEF("v2+ RSDP, usingying 64 bit XSDT address %#" PRIx64 "\n", acpi.rsdp->xsdt_address);
    acpi.sdt_pa = acpi.rsdp->xsdt_address;
  }

  // map the *sdt somewhere
  const size_t sdt_area_len = 0x1000; // 4K should cover it. TODO: see if it's specced
  const void * const sdt_ptr = map_region(acpi.sdt_pa, sdt_area_len, "acpi sdt area");
  if (!sdt_ptr) {
    dprintf(INFO, "ACPI LITE: failed to map SDT address %#" PRIxPTR " to virtual\n", acpi.sdt_pa);
    return ERR_NOT_FOUND;
  }
  LTRACEF("sdt mapped at %p\n", sdt_ptr);

  auto cleanup_sdt_mapping = lk::make_auto_call([sdt_ptr]() {
      vmm_free_region(vmm_get_kernel_aspace(), ROUNDDOWN((vaddr_t)sdt_ptr, PAGE_SIZE));
      acpi.sdt_pa = 0;
      acpi.sdt = nullptr;
  });

  acpi.sdt = static_cast<const acpi_rsdt_xsdt *>(sdt_ptr);

  if (!validate_sdt(acpi.sdt, &acpi.num_tables, &acpi.xsdt)) {
    dprintf(INFO, "ACPI LITE: RSDT/XSDT structure does not check out\n");
    return ERR_NOT_FOUND;
  }

  dprintf(SPEW, "ACPI LITE: RSDT/XSDT checks out, %zu tables\n", acpi.num_tables);

  // map all of the tables in
  acpi.tables = new const void *[acpi.num_tables];
  for (size_t i = 0; i < acpi.num_tables; i++) {
    status_t err = initialize_table(i);
    if (err < 0) {
      dprintf(INFO, "ACPI LITE: failed to initialize table %zu\n", i);
      // for now, simply continue, the table entry should not be initialized
    }
  }

  // we should be initialized at this point
  cleanup_sdt_mapping.cancel();
  cleanup_rsdp_mapping.cancel();

  if (LOCAL_TRACE) {
    acpi_lite_dump_tables(false);
  }

  return NO_ERROR;
}

void acpi_lite_dump_tables(bool full_dump) {
  if (!acpi.sdt) {
    return;
  }

  printf("root table:\n");
  if (full_dump) {
    hexdump(acpi.sdt, acpi.sdt->header.length);
  }

  // walk the table list
  for (size_t i = 0; i < acpi.num_tables; i++) {
    const auto header = acpi_get_table_at_index(i);
    if (!header) {
      continue;
    }

    printf("table %zu: '%c%c%c%c' len %u\n", i, header->sig[0], header->sig[1], header->sig[2],
           header->sig[3], header->length);
    if (full_dump) {
      hexdump(header, header->length);
    }
  }
}

status_t acpi_process_madt_entries_etc(const uint8_t search_type, const madt_entry_callback callback, void * const cookie) {
  const acpi_madt_table* madt =
      reinterpret_cast<const acpi_madt_table*>(acpi_get_table_by_sig(ACPI_MADT_SIG));
  if (!madt) {
    return ERR_NOT_FOUND;
  }

  // bytewise array of the same table
  const uint8_t* madt_array = reinterpret_cast<const uint8_t*>(madt);

  LTRACEF("table at %p\n", madt_array);

  // walk the table off the end of the header, looking for the requested type
  size_t off = sizeof(*madt);
  while (off < madt->header.length) {
    uint8_t type = madt_array[off];
    uint8_t length = madt_array[off + 1];

    LTRACEF("type %u, length %u\n", type, length);
    if (type == search_type) {
      callback(static_cast<const void*>(&madt_array[off]), length, cookie);
    }

    off += length;
  }

  return NO_ERROR;
}

void acpi_lite_dump_madt_table() {
    auto local_apic_callback = [](const void *_entry, size_t entry_len, void *cookie) {
        const auto *entry = reinterpret_cast<const struct acpi_madt_local_apic_entry *>(_entry);

        printf("\tLOCAL APIC id %d, processor id %d, flags %#x\n",
                entry->apic_id, entry->processor_id, entry->flags);
    };

    auto io_apic_callback = [](const void *_entry, size_t entry_len, void *cookie) {
        const auto *entry = reinterpret_cast<const struct acpi_madt_io_apic_entry *>(_entry);

        printf("\tIO APIC id %d, address %#x gsi base %u\n",
                entry->io_apic_id, entry->io_apic_address, entry->global_system_interrupt_base);
    };

    auto int_source_override_callback = [](const void *_entry, size_t entry_len, void *cookie) {
        const auto *entry = reinterpret_cast<const struct acpi_madt_int_source_override_entry *>(_entry);

        printf("\tINT OVERRIDE bus %u, source %u, gsi %u, flags %#x\n",
                entry->bus, entry->source, entry->global_sys_interrupt, entry->flags);
    };
    printf("MADT/APIC table:\n");
    acpi_process_madt_entries_etc(ACPI_MADT_TYPE_LOCAL_APIC, local_apic_callback, nullptr);
    acpi_process_madt_entries_etc(ACPI_MADT_TYPE_IO_APIC, io_apic_callback, nullptr);
    acpi_process_madt_entries_etc(ACPI_MADT_TYPE_INT_SOURCE_OVERRIDE, int_source_override_callback, nullptr);
}

// vim: set ts=2 sw=2 expandtab:
