#include <lk/reg.h>
#include <platform/bcm28xx.h>
#include <platform/bcm28xx/pll.h>
#include <stdint.h>

struct pll_stage1 {
  volatile uint32_t *ana;
  uint32_t enable_bit; // the bit to enable it within A2W_XOSC_CTRL
  volatile uint32_t *frac;
  volatile uint32_t *ctrl;
};

static struct pll_stage1 plla = {
  .ana = REG32(A2W_PLLA_ANA0),
  .enable_bit = A2W_XOSC_CTRL_PLLAEN_SET,
  .frac = REG32(A2W_PLLA_FRAC),
  .ctrl = REG32(A2W_PLLA_CTRL)
};

static struct pll_stage1 pllb = {
  .ana = REG32(A2W_PLLB_ANA0),
  .enable_bit = A2W_XOSC_CTRL_PLLBEN_SET,
  .frac = REG32(A2W_PLLB_FRAC),
  .ctrl = REG32(A2W_PLLB_CTRL)
};

void configure_pll_b(uint32_t freq) {
  *REG32(A2W_XOSC_CTRL) |= A2W_PASSWORD | pllb.enable_bit;
  *pllb.frac = A2W_PASSWORD | 0xeaaa8; // out of 0x100000
  *pllb.ctrl = A2W_PASSWORD | 48 | 0x1000;
  *REG32(CM_PLLB) = CM_PASSWORD | CM_PLLB_DIGRST_SET | CM_PLLB_ANARST_SET;
  *REG32(CM_PLLB) = CM_PASSWORD | CM_PLLB_DIGRST_SET | CM_PLLB_ANARST_SET | CM_PLLB_HOLDARM_SET;

  pllb.ana[3] = A2W_PASSWORD | 0x100;
  pllb.ana[2] = A2W_PASSWORD | 0x0;
  pllb.ana[1] = A2W_PASSWORD | 0x140000;
  pllb.ana[0] = A2W_PASSWORD | 0x0;

  // set dig values

  *REG32(A2W_PLLB_ARM) = A2W_PASSWORD | 2;

  *REG32(CM_PLLB) = CM_PASSWORD | CM_PLLB_DIGRST_SET | CM_PLLB_ANARST_SET | CM_PLLB_HOLDARM_SET | CM_PLLB_LOADARM_SET;
  *REG32(CM_PLLB) = CM_PASSWORD | CM_PLLB_DIGRST_SET | CM_PLLB_ANARST_SET | CM_PLLB_HOLDARM_SET;
  *REG32(CM_PLLB) = CM_PASSWORD;

  *REG32(CM_ARMCTL) = CM_PASSWORD | 4 | CM_ARMCTL_ENAB_SET;
}
