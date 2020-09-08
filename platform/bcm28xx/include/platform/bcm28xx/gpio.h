#pragma once

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

enum BCM2708PinmuxSetting {
  kBCM2708PinmuxIn = 0,
  kBCM2708PinmuxOut = 1,
  kBCM2708Pinmux_ALT5 = 2,
  kBCM2708Pinmux_ALT4 = 3,
  kBCM2708Pinmux_ALT0 = 4,
  kBCM2708Pinmux_ALT1 = 5,
  kBCM2708Pinmux_ALT2 = 6,
  kBCM2708Pinmux_ALT3 = 7
};

enum pull_mode {
  kPullOff = 0,
  kPullDown = 1,
  kPullUp = 2
};

#ifndef RPI4
struct gpio_pull_batch {
  uint32_t bank[3][2];
};

#define GPIO_PULL_CLEAR(batch) { bzero(&batch, sizeof(batch)); }
#define GPIO_PULL_SET(batch, pin, mode) { batch.bank[mode][pin / 32] |= 1 << (pin % 32); }

void gpio_apply_batch(struct gpio_pull_batch *batch);
#else
struct gpio_pull_batch {
  uint32_t bank[4];
  uint32_t mask[4];
};
#define GPIO_PULL_CLEAR(batch)
#define GPIO_PULL_SET(batch, pin, mode)
static inline void gpio_apply_batch(struct gpio_pull_batch *batch) {}
#endif

void gpio_set_pull(unsigned nr, enum pull_mode mode);

#ifdef __cplusplus
}
#endif
