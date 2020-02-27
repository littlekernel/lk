#include <lk/reg.h>
#include <platform/bcm28xx.h>
#include <platform/bcm28xx/pll.h>
#include <stdint.h>

const struct pll_def pll_def[] = {
  [PLL_A] = {
    .name = "PLLA",
    .ana = REG32(A2W_PLLA_ANA0),
    .enable_bit = A2W_XOSC_CTRL_PLLAEN_SET,
    .frac = REG32(A2W_PLLA_FRAC),
    .ctrl = REG32(A2W_PLLA_CTRL),
    .ndiv_mask = A2W_PLLA_CTRL_NDIV_SET,
    .pdiv_mask = A2W_PLLA_CTRL_PDIV_SET,
    .pdiv_shift = A2W_PLLA_CTRL_PDIV_LSB,
    .ana1_pdiv_bit = 14,
  },
  [PLL_B] = {
    .name = "PLLB",
    .ana = REG32(A2W_PLLB_ANA0),
    .enable_bit = A2W_XOSC_CTRL_PLLBEN_SET,
    .frac = REG32(A2W_PLLB_FRAC),
    .ctrl = REG32(A2W_PLLB_CTRL),
    .ndiv_mask = A2W_PLLB_CTRL_NDIV_SET,
    .pdiv_mask = A2W_PLLB_CTRL_PDIV_SET,
    .pdiv_shift = A2W_PLLB_CTRL_PDIV_LSB,
    .ana1_pdiv_bit = 14,
  },
  [PLL_C] = {
    .name = "PLLC",
    .ana = REG32(A2W_PLLC_ANA0),
    .enable_bit = A2W_XOSC_CTRL_PLLCEN_SET,
    .frac = REG32(A2W_PLLC_FRAC),
    .ctrl = REG32(A2W_PLLC_CTRL),
    .ndiv_mask = A2W_PLLC_CTRL_NDIV_SET,
    .pdiv_mask = A2W_PLLC_CTRL_PDIV_SET,
    .pdiv_shift = A2W_PLLC_CTRL_PDIV_LSB,
    .ana1_pdiv_bit = 14,
  },
  [PLL_D] = {
    .name = "PLLD",
    .ana = REG32(A2W_PLLD_ANA0),
    .enable_bit = A2W_XOSC_CTRL_PLLDEN_SET,
    .frac = REG32(A2W_PLLD_FRAC),
    .ctrl = REG32(A2W_PLLD_CTRL),
    .ndiv_mask = A2W_PLLD_CTRL_NDIV_SET,
    .pdiv_mask = A2W_PLLD_CTRL_PDIV_SET,
    .pdiv_shift = A2W_PLLD_CTRL_PDIV_LSB,
    .ana1_pdiv_bit = 14,
  },
  [PLL_H] = {
    .name = "PLLH",
    .ana = REG32(A2W_PLLH_ANA0),
    .enable_bit = A2W_XOSC_CTRL_PLLCEN_SET, // official firmware does this (?)
    .frac = REG32(A2W_PLLH_FRAC),
    .ctrl = REG32(A2W_PLLH_CTRL),
    .ndiv_mask = A2W_PLLH_CTRL_NDIV_SET,
    .pdiv_mask = A2W_PLLH_CTRL_PDIV_SET,
    .pdiv_shift = A2W_PLLH_CTRL_PDIV_LSB,
    .ana1_pdiv_bit = 11,
  },
};

const struct pll_chan_def pll_chan_def[] = {
  [PLL_CHAN_ACORE] = {
    .name = "PLLA_CORE",
    .ctrl = REG32(A2W_PLLA_CORE),
    .chenb_bit = A2W_PLLA_CORE_CHENB_LSB,
    .div_mask = A2W_PLLA_CORE_DIV_SET,
    .pll = PLL_A,
  },
  [PLL_CHAN_CCORE0] = {
    .name = "PLLC_CORE0",
    .ctrl = REG32(A2W_PLLC_CORE0),
    .chenb_bit = A2W_PLLC_CORE0_CHENB_LSB,
    .div_mask = A2W_PLLC_CORE0_DIV_SET,
    .pll = PLL_C,
  },
  [PLL_CHAN_CCORE1] = {
    .name = "PLLC_CORE1",
    .ctrl = REG32(A2W_PLLC_CORE1),
    .chenb_bit = A2W_PLLC_CORE1_CHENB_LSB,
    .div_mask = A2W_PLLC_CORE1_DIV_SET,
    .pll = PLL_C,
  },
  [PLL_CHAN_CCORE2] = {
    .name = "PLLC_CORE2",
    .ctrl = REG32(A2W_PLLC_CORE2),
    .chenb_bit = A2W_PLLC_CORE2_CHENB_LSB,
    .div_mask = A2W_PLLC_CORE2_DIV_SET,
    .pll = PLL_C,
  },
  [PLL_CHAN_DPER] = {
    .name = "PLLD_PER",
    .ctrl = REG32(A2W_PLLD_PER),
    .chenb_bit = A2W_PLLD_PER_CHENB_LSB,
    .div_mask = A2W_PLLD_PER_DIV_SET,
    .pll = PLL_D,
  },
  [PLL_CHAN_HAUX] = {
    .name = "PLLH_AUX",
    .ctrl = REG32(A2W_PLLH_AUX),
    .chenb_bit = A2W_PLLH_AUX_CHENB_LSB,
    .div_mask = A2W_PLLH_AUX_DIV_SET,
    .pll = PLL_H,
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
