#include "configuration_table.h"
#include "boot_service_provider.h"
#include "system_table.h"
#include <string.h>

void setup_configuration_table(EfiSystemTable *table) {
  auto &rng = table->configuration_table[0];
  rng.vendor_guid = LINUX_EFI_RANDOM_SEED_TABLE_GUID;
  rng.vendor_table = alloc_page(PAGE_SIZE);
  auto rng_seed = reinterpret_cast<linux_efi_random_seed *>(rng.vendor_table);
  rng_seed->size = 512;
  memset(&rng_seed->bits, 0, rng_seed->size);
  table->number_of_table_entries = 1;
}