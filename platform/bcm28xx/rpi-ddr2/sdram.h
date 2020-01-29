#pragma once

void sdram_init(void);

enum RamSize {
  kRamSize1GB = 0,
  kRamSize512MB = 1,
  kRamSize256MB = 2,
  kRamSize128MB = 3,
  kRamSize2GB = 4,
  kRamSize4GB = 5,
  kRamSizeUnknown
};
