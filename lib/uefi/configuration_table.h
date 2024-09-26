#ifndef __CONFIGURATION_TABLE_
#define __CONFIGURATION_TABLE_

#include "system_table.h"
#include "types.h"

struct linux_efi_random_seed {
  uint32_t size;
  uint8_t bits[];
};

static constexpr auto LINUX_EFI_RANDOM_SEED_TABLE_GUID =
    EfiGuid{0x1ce1e5bc,
            0x7ceb,
            0x42f2,
            {0x81, 0xe5, 0x8a, 0xad, 0xf1, 0x80, 0xf5, 0x7b}};

void setup_configuration_table(EfiSystemTable *table);

#endif
