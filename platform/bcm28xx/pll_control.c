#include <lk/reg.h>
#include <platform/bcm28xx.h>
#include <platform/bcm28xx/pll.h>
#include <stdint.h>

const struct pll_def pll_def[] = {
  [PLL_A] = {
    .name = "PLLA",
    .ana = REG32(A2W_PLLA_ANA0),
    .dig = REG32(A2W_PLLA_DIG0),
    .enable_bit = A2W_XOSC_CTRL_PLLAEN_SET,
    .frac = REG32(A2W_PLLA_FRAC),
    .ctrl = REG32(A2W_PLLA_CTRL),
    .ndiv_mask = A2W_PLLA_CTRL_NDIV_SET,
    .ana1_prescale_bit = 14,
    .cm_pll = REG32(CM_PLLA),
  },
  [PLL_B] = {
    .name = "PLLB",
    .ana = REG32(A2W_PLLB_ANA0),
    .dig = REG32(A2W_PLLB_DIG0),
    .enable_bit = A2W_XOSC_CTRL_PLLBEN_SET,
    .frac = REG32(A2W_PLLB_FRAC),
    .ctrl = REG32(A2W_PLLB_CTRL),
    .ndiv_mask = A2W_PLLB_CTRL_NDIV_SET,
    .ana1_prescale_bit = 14,
    .cm_pll = REG32(CM_PLLB),
  },
  [PLL_C] = {
    .name = "PLLC",
    .ana = REG32(A2W_PLLC_ANA0),
    .dig = REG32(A2W_PLLC_DIG0),
    .enable_bit = A2W_XOSC_CTRL_PLLCEN_SET,
    .frac = REG32(A2W_PLLC_FRAC),
    .ctrl = REG32(A2W_PLLC_CTRL),
    .ndiv_mask = A2W_PLLC_CTRL_NDIV_SET,
    .ana1_prescale_bit = 14,
    .cm_pll = REG32(CM_PLLC),
  },
  [PLL_D] = {
    .name = "PLLD",
    .ana = REG32(A2W_PLLD_ANA0),
    .dig = REG32(A2W_PLLD_DIG0),
    .enable_bit = A2W_XOSC_CTRL_PLLDEN_SET,
    .frac = REG32(A2W_PLLD_FRAC),
    .ctrl = REG32(A2W_PLLD_CTRL),
    .ndiv_mask = A2W_PLLD_CTRL_NDIV_SET,
    .ana1_prescale_bit = 14,
    .cm_pll = REG32(CM_PLLD),
  },
  [PLL_H] = {
    .name = "PLLH",
    .ana = REG32(A2W_PLLH_ANA0),
    .dig = REG32(A2W_PLLH_DIG0),
    .enable_bit = A2W_XOSC_CTRL_PLLCEN_SET, // official firmware does this (?)
    .frac = REG32(A2W_PLLH_FRAC),
    .ctrl = REG32(A2W_PLLH_CTRL),
    .ndiv_mask = A2W_PLLH_CTRL_NDIV_SET,
    .ana1_prescale_bit = 11,
    .cm_pll = REG32(CM_PLLH),
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
  [PLL_CHAN_APER] = {
    .name = "PLLA_PER",
    .ctrl = REG32(A2W_PLLA_PER),
    .chenb_bit = A2W_PLLA_PER_CHENB_LSB,
    .div_mask = A2W_PLLA_PER_DIV_SET,
    .pll = PLL_A,
  },
  [PLL_CHAN_ADSI0] = {
    .name = "PLLA_DSI0",
    .ctrl = REG32(A2W_PLLA_DSI0),
    .chenb_bit = A2W_PLLA_DSI0_CHENB_LSB,
    .div_mask = A2W_PLLA_DSI0_DIV_SET,
    .pll = PLL_A,
  },
  [PLL_CHAN_ACCP2] = {
    .name = "PLLA_CCP2",
    .ctrl = REG32(A2W_PLLA_CCP2),
    .chenb_bit = A2W_PLLA_CCP2_CHENB_LSB,
    .div_mask = A2W_PLLA_CCP2_DIV_SET,
    .pll = PLL_A,
  },
  [PLL_CHAN_BARM] = {
    .name = "PLLB_ARM",
    .ctrl = REG32(A2W_PLLB_ARM),
    .chenb_bit = A2W_PLLB_ARM_CHENB_LSB,
    .div_mask = A2W_PLLB_ARM_DIV_SET,
    .pll = PLL_B,
  },
  [PLL_CHAN_BSP0] = {
    .name = "PLLB_SP0",
    .ctrl = REG32(A2W_PLLB_SP0),
    .chenb_bit = A2W_PLLB_SP0_CHENB_LSB,
    .div_mask = A2W_PLLB_SP0_DIV_SET,
    .pll = PLL_B,
  },
  [PLL_CHAN_BSP1] = {
    .name = "PLLB_SP1",
    .ctrl = REG32(A2W_PLLB_SP1),
    .chenb_bit = A2W_PLLB_SP1_CHENB_LSB,
    .div_mask = A2W_PLLB_SP1_DIV_SET,
    .pll = PLL_B,
  },
  [PLL_CHAN_BSP2] = {
    .name = "PLLB_SP2",
    .ctrl = REG32(A2W_PLLB_SP2),
    .chenb_bit = A2W_PLLB_SP2_CHENB_LSB,
    .div_mask = A2W_PLLB_SP2_DIV_SET,
    .pll = PLL_B,
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
  [PLL_CHAN_CPER] = {
    .name = "PLLC_PER",
    .ctrl = REG32(A2W_PLLC_PER),
    .chenb_bit = A2W_PLLC_PER_CHENB_LSB,
    .div_mask = A2W_PLLC_PER_DIV_SET,
    .pll = PLL_C,
  },
  [PLL_CHAN_DCORE] = {
    .name = "PLLD_CORE",
    .ctrl = REG32(A2W_PLLD_CORE),
    .chenb_bit = A2W_PLLD_CORE_CHENB_LSB,
    .div_mask = A2W_PLLD_CORE_DIV_SET,
    .pll = PLL_D,
  },
  [PLL_CHAN_DPER] = {
    .name = "PLLD_PER",
    .ctrl = REG32(A2W_PLLD_PER),
    .chenb_bit = A2W_PLLD_PER_CHENB_LSB,
    .div_mask = A2W_PLLD_PER_DIV_SET,
    .pll = PLL_D,
  },
  [PLL_CHAN_DDSI0] = {
    .name = "PLLD_DSI0",
    .ctrl = REG32(A2W_PLLD_DSI0),
    .chenb_bit = A2W_PLLD_DSI0_CHENB_LSB,
    .div_mask = A2W_PLLD_DSI0_DIV_SET,
    .pll = PLL_D,
  },
  [PLL_CHAN_DDSI1] = {
    .name = "PLLD_DSI1",
    .ctrl = REG32(A2W_PLLD_DSI1),
    .chenb_bit = A2W_PLLD_DSI1_CHENB_LSB,
    .div_mask = A2W_PLLD_DSI1_DIV_SET,
    .pll = PLL_D,
  },
  [PLL_CHAN_HPIX] = {
    .name = "PLLH_PIX",
    .ctrl = REG32(A2W_PLLH_PIX),
    .chenb_bit = A2W_PLLH_PIX_CHENB_LSB,
    .div_mask = A2W_PLLH_PIX_DIV_SET,
    .pll = PLL_H,
  },
  [PLL_CHAN_HRCAL] = {
    .name = "PLLH_RCAL",
    .ctrl = REG32(A2W_PLLH_RCAL),
    .chenb_bit = A2W_PLLH_RCAL_CHENB_LSB,
    .div_mask = A2W_PLLH_RCAL_DIV_SET,
    .pll = PLL_H,
  },
  [PLL_CHAN_HAUX] = {
    .name = "PLLH_AUX",
    .ctrl = REG32(A2W_PLLH_AUX),
    .chenb_bit = A2W_PLLH_AUX_CHENB_LSB,
    .div_mask = A2W_PLLH_AUX_DIV_SET,
    .pll = PLL_H,
  },
};

static void pll_start(enum pll pll)
{
  const struct pll_def *def = &pll_def[pll];
  uint32_t dig[4];

  dig[3] = def->dig[3];
  dig[2] = def->dig[2];
  dig[1] = def->dig[1];
  dig[0] = def->dig[0];

  if (pll == PLL_H) {
    // CM_PLLH does not contain any HOLD bits
    def->dig[3] = A2W_PASSWORD | dig[3];
    def->dig[2] = A2W_PASSWORD | (dig[2] & 0xffffff57);
    def->dig[1] = A2W_PASSWORD | dig[1];
  } else {
    // set all HOLD bits
    *def->cm_pll = CM_PASSWORD | (*def->cm_pll | 0xaa);
    def->dig[3] = A2W_PASSWORD | dig[3];
    def->dig[2] = A2W_PASSWORD | (dig[2] & 0xffeffbfe);
    def->dig[1] = A2W_PASSWORD | (dig[1] & 0xffffbfff);
  }
  def->dig[0] = A2W_PASSWORD | dig[0];

  *def->ctrl = A2W_PASSWORD | (*def->ctrl | A2W_PLL_CTRL_PRSTN);

  def->dig[3] = A2W_PASSWORD | (dig[3] | 0x42);
  def->dig[2] = A2W_PASSWORD | dig[2];
  def->dig[1] = A2W_PASSWORD | dig[1];
  def->dig[0] = A2W_PASSWORD | dig[0];
}

void configure_pll_b(uint32_t freq) {
  const struct pll_def *def = &pll_def[PLL_B];
  uint32_t orig_ctrl = *def->ctrl;
  *REG32(A2W_XOSC_CTRL) |= A2W_PASSWORD | def->enable_bit;
  *def->frac = A2W_PASSWORD | 0xeaaa8; // out of 0x100000
  *def->ctrl = A2W_PASSWORD | 48 | 0x1000;
  *REG32(CM_PLLB) = CM_PASSWORD | CM_PLL_DIGRST | CM_PLL_ANARST;
  *REG32(CM_PLLB) = CM_PASSWORD | CM_PLL_DIGRST | CM_PLL_ANARST | CM_PLLB_HOLDARM_SET;

  def->ana[3] = A2W_PASSWORD | 0x100;
  def->ana[2] = A2W_PASSWORD | 0x0;
  def->ana[1] = A2W_PASSWORD | 0x140000;
  def->ana[0] = A2W_PASSWORD | 0x0;

  // set dig values

  *REG32(A2W_PLLB_ARM) = A2W_PASSWORD | 2;

  *REG32(CM_PLLB) = CM_PASSWORD | CM_PLL_DIGRST | CM_PLL_ANARST | CM_PLLB_HOLDARM_SET | CM_PLLB_LOADARM_SET;
  *REG32(CM_PLLB) = CM_PASSWORD | CM_PLL_DIGRST | CM_PLL_ANARST | CM_PLLB_HOLDARM_SET;
  *REG32(CM_PLLB) = CM_PASSWORD;

  *REG32(CM_ARMCTL) = CM_PASSWORD | 4 | CM_ARMCTL_ENAB_SET;

  if ((orig_ctrl & A2W_PLL_CTRL_PRSTN) == 0)
    pll_start(PLL_B);
}
