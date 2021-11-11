// Copyright 2020 The Fuchsia Authors
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#include "lib/acpi_lite.h"

#include <inttypes.h>
#include <trace.h>
#include <zircon/compiler.h>

#include <pretty/hexdump.h>
#include <vm/physmap.h>

#define LOCAL_TRACE 0

// global state of the acpi lite library
struct acpi_lite_state {
  const acpi_rsdp* rsdp;
  const acpi_rsdt_xsdt* sdt;
  size_t num_tables;  // number of top level tables
  bool xsdt;          // are the pointers 64 or 32bit?
} acpi;

static const void* phys_to_ptr(uintptr_t pa) {
  if (!is_physmap_phys_addr(pa)) {
    return nullptr;
  }
  // consider 0 to be invalid
  if (pa == 0) {
    return nullptr;
  }

  return static_cast<const void*>(paddr_to_physmap(pa));
}

static uint8_t acpi_checksum(const void* _buf, size_t len) {
  uint8_t c = 0;

  const uint8_t* buf = static_cast<const uint8_t*>(_buf);
  for (size_t i = 0; i < len; i++) {
    c = (uint8_t)(c + buf[i]);
  }

  return c;
}

static bool validate_rsdp(const acpi_rsdp* rsdp) {
  // check the signature
  if (memcmp(ACPI_RSDP_SIG, rsdp->sig, 8)) {
    return false;
  }

  // validate the v1 checksum on the first 20 bytes of the table
  uint8_t c = acpi_checksum(rsdp, 20);
  if (c) {
    return false;
  }

  // is it v2?
  if (rsdp->revision >= 2) {
    if (rsdp->length < 36 || rsdp->length > 4096) {
      // keep the table length within reason
      return false;
    }

    c = acpi_checksum(rsdp, rsdp->length);
    if (c) {
      return false;
    }
  }

  // seems okay
  return true;
}

static zx_paddr_t find_rsdp_pc() {
  // search for it in the BIOS EBDA area (0xe0000..0xfffff) on 16 byte boundaries
  for (zx_paddr_t ptr = 0xe0000; ptr <= 0xfffff; ptr += 16) {
    const auto rsdp = static_cast<const acpi_rsdp*>(phys_to_ptr(ptr));

    if (validate_rsdp(rsdp)) {
      return ptr;
    }
  }

  return 0;
}

static bool validate_sdt(const acpi_rsdt_xsdt* sdt, size_t* num_tables, bool* xsdt) {
  LTRACEF("%p\n", sdt);

  // bad pointer
  if (!sdt) {
    return false;
  }

  // check the signature and see if it's a rsdt or xsdt
  if (!memcmp(sdt->header.sig, "XSDT", 4)) {
    *xsdt = true;
  } else if (!memcmp(sdt->header.sig, "RSDT", 4)) {
    *xsdt = false;
  } else {
    return false;
  }

  // is the length sane?
  if (sdt->header.length < 36 || sdt->header.length > 4096) {
    return false;
  }

  // is it a revision we understand?
  if (sdt->header.revision != 1) {
    return false;
  }

  // checksum the entire table
  uint8_t c = acpi_checksum(sdt, sdt->header.length);
  if (c) {
    return false;
  }

  // compute the number of pointers to tables we have
  *num_tables = (sdt->header.length - 36u) / (*xsdt ? 8u : 4u);

  // looks okay
  return true;
}

const acpi_sdt_header* acpi_get_table_at_index(size_t index) {
  if (index >= acpi.num_tables) {
    return nullptr;
  }

  zx_paddr_t pa;
  if (acpi.xsdt) {
    pa = acpi.sdt->addr64[index];
  } else {
    pa = acpi.sdt->addr32[index];
  }

  return static_cast<const acpi_sdt_header*>(phys_to_ptr(pa));
}

const acpi_sdt_header* acpi_get_table_by_sig(const char* sig) {
  // walk the list of tables
  for (size_t i = 0; i < acpi.num_tables; i++) {
    const auto header = acpi_get_table_at_index(i);
    if (!header) {
      continue;
    }

    if (!memcmp(sig, header->sig, 4)) {
      // validate the checksum
      uint8_t c = acpi_checksum(header, header->length);
      if (c == 0) {
        return header;
      }
    }
  }

  return nullptr;
}

zx_status_t acpi_lite_init(zx_paddr_t rsdp_pa) {
  LTRACEF("passed in rsdp %#" PRIxPTR "\n", rsdp_pa);

  // see if the rsdp pointer is valid
  if (rsdp_pa == 0) {
    // search around for it in a platform-specific way
#if __x86_64__
    rsdp_pa = find_rsdp_pc();
    if (rsdp_pa == 0) {
      dprintf(INFO, "ACPI LITE: couldn't find ACPI RSDP in BIOS area\n");
    }
#endif

    if (rsdp_pa == 0) {
      return ZX_ERR_NOT_FOUND;
    }
  }

  const void* ptr = phys_to_ptr(rsdp_pa);
  if (!ptr) {
    dprintf(INFO, "ACPI LITE: failed to translate RSDP address %#" PRIxPTR " to virtual\n",
            rsdp_pa);
    return ZX_ERR_NOT_FOUND;
  }

  // see if the RSDP is there
  acpi.rsdp = static_cast<const acpi_rsdp*>(ptr);
  if (!validate_rsdp(acpi.rsdp)) {
    dprintf(INFO, "ACPI LITE: RSDP structure does not check out\n");
    return ZX_ERR_NOT_FOUND;
  }

  dprintf(SPEW, "ACPI LITE: RSDP checks out, found at %#lx\n", rsdp_pa);

  // find the pointer to either the RSDT or XSDT
  acpi.sdt = nullptr;
  if (acpi.rsdp->revision < 2) {
    // v1 RSDP, pointing at a RSDT
    acpi.sdt = static_cast<const acpi_rsdt_xsdt*>(phys_to_ptr(acpi.rsdp->rsdt_address));
  } else {
    // v2+ RSDP, pointing at a XSDT
    // try to use the 64bit address first
    acpi.sdt = static_cast<const acpi_rsdt_xsdt*>(phys_to_ptr(acpi.rsdp->xsdt_address));
    if (!acpi.sdt) {
      acpi.sdt = static_cast<const acpi_rsdt_xsdt*>(phys_to_ptr(acpi.rsdp->rsdt_address));
    }
  }

  if (!validate_sdt(acpi.sdt, &acpi.num_tables, &acpi.xsdt)) {
    dprintf(INFO, "ACPI LITE: RSDT/XSDT structure does not check out\n");
    return ZX_ERR_NOT_FOUND;
  }

  dprintf(SPEW, "ACPI LITE: RSDT/XSDT checks out, %zu tables\n", acpi.num_tables);

  if (LOCAL_TRACE) {
    acpi_lite_dump_tables();
  }

  return ZX_OK;
}

void acpi_lite_dump_tables() {
  if (!acpi.sdt) {
    return;
  }

  printf("root table:\n");
  hexdump(acpi.sdt, acpi.sdt->header.length);

  // walk the table list
  for (size_t i = 0; i < acpi.num_tables; i++) {
    const auto header = acpi_get_table_at_index(i);
    if (!header) {
      continue;
    }

    printf("table %zu: '%c%c%c%c' len %u\n", i, header->sig[0], header->sig[1], header->sig[2],
           header->sig[3], header->length);
    hexdump(header, header->length);
  }
}

zx_status_t acpi_process_madt_entries_etc(const uint8_t search_type,
                                          const MadtEntryCallback& callback) {
  const acpi_madt_table* madt =
      reinterpret_cast<const acpi_madt_table*>(acpi_get_table_by_sig(ACPI_MADT_SIG));
  if (!madt) {
    return ZX_ERR_NOT_FOUND;
  }

  // bytewise array of the same table
  const uint8_t* madt_array = reinterpret_cast<const uint8_t*>(madt);

  // walk the table off the end of the header, looking for the requested type
  size_t off = sizeof(*madt);
  while (off < madt->header.length) {
    uint8_t type = madt_array[off];
    uint8_t length = madt_array[off + 1];

    if (type == search_type) {
      callback(static_cast<const void*>(&madt_array[off]), length);
    }

    off += length;
  }

  return ZX_OK;
}
