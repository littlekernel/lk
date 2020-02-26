#pragma once

enum pll {
  PLL_A,
  PLL_B,
  PLL_C,
  PLL_D,
  PLL_H,

  PLL_NUM,
};

struct pll_def {
  char name[8];
  volatile uint32_t *ana;
  uint32_t enable_bit; // the bit to enable it within A2W_XOSC_CTRL
  volatile uint32_t *frac;
  volatile uint32_t *ctrl;
  uint32_t ndiv_mask;
  uint32_t pdiv_mask;
  unsigned short pdiv_shift;
  unsigned short ana1_pdiv_bit;
};

extern const struct pll_def pll_def[PLL_NUM];

enum pll_chan {
  PLL_CHAN_ACORE,
  PLL_CHAN_APER,
  PLL_CHAN_ADSI0,
  PLL_CHAN_ACCP2,

  PLL_CHAN_BARM,
  PLL_CHAN_BSP0,
  PLL_CHAN_BSP1,
  PLL_CHAN_BSP2,

  PLL_CHAN_CCORE0,
  PLL_CHAN_CCORE1,
  PLL_CHAN_CCORE2,
  PLL_CHAN_CPER,

  PLL_CHAN_DCORE,
  PLL_CHAN_DPER,
  PLL_CHAN_DDSI0,
  PLL_CHAN_DDSI1,

  PLL_CHAN_HPIX,
  PLL_CHAN_HRCAL,
  PLL_CHAN_HAUX,

  PLL_CHAN_NUM,
};

struct pll_chan_def {
  char name[12];
  volatile uint32_t *ctrl;
  int chenb_bit;
  uint32_t div_mask;
  enum pll pll;
};

extern const struct pll_chan_def pll_chan_def[PLL_CHAN_NUM];

#define CM_PLLB                 0x7e101170
#define CM_PLLB_LOADARM_SET                                0x00000001
#define CM_PLLB_HOLDARM_SET                                0x00000002
#define CM_PLLB_ANARST_SET                                 0x00000100
#define CM_PLLB_DIGRST_SET                                 0x00000200
#define CM_ARMCTL               0x7e1011b0
#define CM_ARMCTL_ENAB_SET                                 0x00000010

#define A2W_XOSC_CTRL           0x7e102190
#define A2W_XOSC_CTRL_DDREN_SET                            0x00000010
#define A2W_XOSC_CTRL_PLLAEN_SET                           0x00000040
#define A2W_XOSC_CTRL_PLLBEN_SET                           0x00000080
#define A2W_XOSC_CTRL_PLLCEN_SET                           0x00000001
#define A2W_XOSC_CTRL_PLLDEN_SET                           0x00000020

#define A2W_BASE                (BCM_PERIPH_BASE_VIRT + 0x102000)

#define A2W_PASSWORD                                             0x5a000000

#define A2W_PLLA_ANA0           (A2W_BASE + 0x010)
#define A2W_PLLC_ANA0           (A2W_BASE + 0x030)
#define A2W_PLLD_ANA0           (A2W_BASE + 0x050)
#define A2W_PLLH_ANA0           (A2W_BASE + 0x070)
#define A2W_PLLB_ANA0           (A2W_BASE + 0x0f0)

#define A2W_PLLA_FRAC           (A2W_BASE + 0x200)
#define A2W_PLLC_FRAC           (A2W_BASE + 0x220)
#define A2W_PLLD_FRAC           (A2W_BASE + 0x240)
#define A2W_PLLH_FRAC           (A2W_BASE + 0x260)
#define A2W_PLLB_FRAC           (A2W_BASE + 0x2e0)
#define A2W_PLL_FRAC_MASK                                     0x000fffff

#define A2W_PLLA_CTRL           (A2W_BASE + 0x100)
#define A2W_PLLA_CTRL_PDIV_SET                             0x00007000
#define A2W_PLLA_CTRL_NDIV_SET                             0x000003ff
#define A2W_PLLA_CTRL_PDIV_LSB                             12
#define A2W_PLLC_CTRL           (A2W_BASE + 0x120)
#define A2W_PLLC_CTRL_PDIV_SET                             0x00007000
#define A2W_PLLC_CTRL_NDIV_SET                             0x000003ff
#define A2W_PLLC_CTRL_PDIV_LSB                             12
#define A2W_PLLD_CTRL           (A2W_BASE + 0x140)
#define A2W_PLLD_CTRL_PDIV_SET                             0x00007000
#define A2W_PLLD_CTRL_NDIV_SET                             0x000003ff
#define A2W_PLLD_CTRL_PDIV_LSB                             12
#define A2W_PLLH_CTRL           (A2W_BASE + 0x160)
#define A2W_PLLH_CTRL_PDIV_SET                             0x00007000
#define A2W_PLLH_CTRL_NDIV_SET                             0x000000ff
#define A2W_PLLH_CTRL_PDIV_LSB                             12
#define A2W_PLLB_CTRL           (A2W_BASE + 0x1e0)
#define A2W_PLLB_CTRL_PDIV_SET                             0x00007000
#define A2W_PLLB_CTRL_NDIV_SET                             0x000003ff
#define A2W_PLLB_CTRL_PDIV_LSB                             12

#define A2W_PLLA_DSI0           (A2W_BASE + 0x300)
#define A2W_PLLA_DSI0_CHENB_LSB                            8
#define A2W_PLLA_DSI0_DIV_SET                              0x000000ff
#define A2W_PLLC_CORE2          (A2W_BASE + 0x320)
#define A2W_PLLC_CORE2_CHENB_LSB                           8
#define A2W_PLLC_CORE2_DIV_SET                             0x000000ff
#define A2W_PLLD_DSI0           (A2W_BASE + 0x340)
#define A2W_PLLD_DSI0_CHENB_LSB                            8
#define A2W_PLLD_DSI0_DIV_SET                              0x000000ff
#define A2W_PLLH_AUX            (A2W_BASE + 0x360)
#define A2W_PLLH_AUX_CHENB_LSB                             8
#define A2W_PLLH_AUX_DIV_SET                               0x000000ff
#define A2W_PLLB_ARM            (A2W_BASE + 0x3e0)
#define A2W_PLLB_ARM_CHENB_LSB                             8
#define A2W_PLLB_ARM_DIV_SET                               0x000000ff
#define A2W_PLLA_CORE           (A2W_BASE + 0x400)
#define A2W_PLLA_CORE_CHENB_LSB                            8
#define A2W_PLLA_CORE_DIV_SET                              0x000000ff
#define A2W_PLLD_CORE           (A2W_BASE + 0x440)
#define A2W_PLLD_CORE_CHENB_LSB                            8
#define A2W_PLLD_CORE_DIV_SET                              0x000000ff
#define A2W_PLLH_RCAL           (A2W_BASE + 0x460)
#define A2W_PLLH_RCAL_CHENB_LSB                            8
#define A2W_PLLH_RCAL_DIV_SET                              0x000000ff
#define A2W_PLLB_SP0            (A2W_BASE + 0x4e0)
#define A2W_PLLB_SP0_CHENB_LSB                             8
#define A2W_PLLB_SP0_DIV_SET                               0x000000ff
#define A2W_PLLA_PER            (A2W_BASE + 0x500)
#define A2W_PLLA_PER_CHENB_LSB                             8
#define A2W_PLLA_PER_DIV_SET                               0x000000ff
#define A2W_PLLC_PER            (A2W_BASE + 0x520)
#define A2W_PLLC_PER_CHENB_LSB                             8
#define A2W_PLLC_PER_DIV_SET                               0x000000ff
#define A2W_PLLD_PER            (A2W_BASE + 0x540)
#define A2W_PLLD_PER_CHENB_LSB                             8
#define A2W_PLLD_PER_DIV_SET                               0x000000ff
#define A2W_PLLH_PIX            (A2W_BASE + 0x560)
#define A2W_PLLH_PIX_CHENB_LSB                             8
#define A2W_PLLH_PIX_DIV_SET                               0x000000ff
#define A2W_PLLC_CORE1          (A2W_BASE + 0x420)
#define A2W_PLLC_CORE1_CHENB_LSB                           8
#define A2W_PLLC_CORE1_DIV_SET                             0x000000ff
#define A2W_PLLB_SP1            (A2W_BASE + 0x5e0)
#define A2W_PLLB_SP1_CHENB_LSB                             8
#define A2W_PLLB_SP1_DIV_SET                               0x000000ff
#define A2W_PLLA_CCP2           (A2W_BASE + 0x600)
#define A2W_PLLA_CCP2_CHENB_LSB                            8
#define A2W_PLLA_CCP2_DIV_SET                              0x000000ff
#define A2W_PLLC_CORE0          (A2W_BASE + 0x620)
#define A2W_PLLC_CORE0_CHENB_LSB                           8
#define A2W_PLLC_CORE0_DIV_SET                             0x000000ff
#define A2W_PLLD_DSI1           (A2W_BASE + 0x640)
#define A2W_PLLD_DSI1_CHENB_LSB                            8
#define A2W_PLLD_DSI1_DIV_SET                              0x000000ff
#define A2W_PLLB_SP2            (A2W_BASE + 0x6e0)
#define A2W_PLLB_SP2_CHENB_LSB                             8
#define A2W_PLLB_SP2_DIV_SET                               0x000000ff
