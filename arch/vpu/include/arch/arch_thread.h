#pragma once

#include <sys/types.h>

struct arch_thread {
  uint32_t sp;
  uint32_t sr;
};
