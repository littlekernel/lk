#include <lk/reg.h>
#include <platform/bcm28xx.h>
#include <stdio.h>

void print_timestamp() {
  uint32_t clock_lo = *REG32(ST_CLO);

  printf("%3d.%06d ", clock_lo / 1000000, clock_lo % 1000000);
}
