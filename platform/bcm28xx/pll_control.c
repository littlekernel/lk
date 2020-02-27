#include <lk/reg.h>
#include <platform/bcm28xx.h>
#include <platform/bcm28xx/pll.h>
#include <stdint.h>

const struct pll_def pll_def[] = {
  [PLL_A] = {
    .ana = REG32(A2W_PLLA_ANA0),
    .enable_bit = A2W_XOSC_CTRL_PLLAEN_SET,
    .frac = REG32(A2W_PLLA_FRAC),
    .ctrl = REG32(A2W_PLLA_CTRL),
  },
  [PLL_B] = {
    .ana = REG32(A2W_PLLB_ANA0),
    .enable_bit = A2W_XOSC_CTRL_PLLBEN_SET,
    .frac = REG32(A2W_PLLB_FRAC),
    .ctrl = REG32(A2W_PLLB_CTRL),
  },
  [PLL_C] = {
    .ana = REG32(A2W_PLLC_ANA0),
    .enable_bit = A2W_XOSC_CTRL_PLLCEN_SET,
    .frac = REG32(A2W_PLLC_FRAC),
    .ctrl = REG32(A2W_PLLC_CTRL),
  },
  [PLL_D] = {
    .ana = REG32(A2W_PLLD_ANA0),
    .enable_bit = A2W_XOSC_CTRL_PLLDEN_SET,
    .frac = REG32(A2W_PLLD_FRAC),
    .ctrl = REG32(A2W_PLLD_CTRL),
  },
  [PLL_H] = {
    .ana = REG32(A2W_PLLH_ANA0),
    .enable_bit = A2W_XOSC_CTRL_PLLCEN_SET, // official firmware does this (?)
    .frac = REG32(A2W_PLLH_FRAC),
    .ctrl = REG32(A2W_PLLH_CTRL),
  },
};

void configure_pll_b(uint32_t freq) {
  const struct pll_def *def = &pll_def[PLL_B];
  *REG32(A2W_XOSC_CTRL) |= A2W_PASSWORD | def->enable_bit;
  *def->frac = A2W_PASSWORD | 0xeaaa8; // out of 0x100000
  *def->ctrl = A2W_PASSWORD | 48 | 0x1000;
  *REG32(CM_PLLB) = CM_PASSWORD | CM_PLLB_DIGRST_SET | CM_PLLB_ANARST_SET;
  *REG32(CM_PLLB) = CM_PASSWORD | CM_PLLB_DIGRST_SET | CM_PLLB_ANARST_SET | CM_PLLB_HOLDARM_SET;

  def->ana[3] = A2W_PASSWORD | 0x100;
  def->ana[2] = A2W_PASSWORD | 0x0;
  def->ana[1] = A2W_PASSWORD | 0x140000;
  def->ana[0] = A2W_PASSWORD | 0x0;

  // set dig values

  *REG32(A2W_PLLB_ARM) = A2W_PASSWORD | 2;

  *REG32(CM_PLLB) = CM_PASSWORD | CM_PLLB_DIGRST_SET | CM_PLLB_ANARST_SET | CM_PLLB_HOLDARM_SET | CM_PLLB_LOADARM_SET;
  *REG32(CM_PLLB) = CM_PASSWORD | CM_PLLB_DIGRST_SET | CM_PLLB_ANARST_SET | CM_PLLB_HOLDARM_SET;
  *REG32(CM_PLLB) = CM_PASSWORD;

  *REG32(CM_ARMCTL) = CM_PASSWORD | 4 | CM_ARMCTL_ENAB_SET;
}
