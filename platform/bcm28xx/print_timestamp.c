#include <lk/reg.h>
#include <platform/bcm28xx.h>
#include <stdio.h>

void print_timestamp() {
  uint32_t clock_lo = *REG32(ST_CLO);

  printf("%3ld.%06ld ", clock_lo / 1000000, clock_lo % 1000000);
}
