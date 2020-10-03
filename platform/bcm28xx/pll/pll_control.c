#include <lk/reg.h>
#include <lk/bits.h>
#include <lk/console_cmd.h>
#include <platform/bcm28xx.h>
#include <platform/bcm28xx/pll.h>
#include <stdint.h>

#define PLL_MAX_LOCKWAIT 1000

static int cmd_set_pll_freq(int argc, const cmd_args *argv);

STATIC_COMMAND_START
STATIC_COMMAND("set_pll_freq", "set pll frequency", &cmd_set_pll_freq)
STATIC_COMMAND_END(pll_control);

const struct pll_def pll_def[] = {
#if 0
  [PLL_A] = {
    .name = "PLLA",
    .ana = REG32(A2W_PLLA_ANA0),
    .dig = REG32(A2W_PLLA_DIG0),
    .enable_bit = A2W_XOSC_CTRL_PLLAEN_SET,
    .frac = REG32(A2W_PLLA_FRAC),
    .ctrl = REG32(A2W_PLLA_CTRL),
    .ndiv_mask = A2W_PLLA_CTRL_NDIV_SET,
    .ana1_prescale_bit = 14,
    .cm_flock_bit = CM_LOCK_FLOCKA_BIT,
    .cm_pll = REG32(CM_PLLA),
    .ana_kaip = REG32(A2W_PLLA_ANA_KAIP),
    .ana_vco = REG32(A2W_PLLA_ANA_VCO),
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
    .cm_flock_bit = CM_LOCK_FLOCKB_BIT,
    .cm_pll = REG32(CM_PLLB),
    .ana_kaip = REG32(A2W_PLLB_ANA_KAIP),
    .ana_vco = REG32(A2W_PLLB_ANA_VCO),
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
    .cm_flock_bit = CM_LOCK_FLOCKC_BIT,
    .cm_pll = REG32(CM_PLLC),
    .ana_kaip = REG32(A2W_PLLC_ANA_KAIP),
    .ana_vco = REG32(A2W_PLLC_ANA_VCO),
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
    .cm_flock_bit = CM_LOCK_FLOCKD_BIT,
    .cm_pll = REG32(CM_PLLD),
    .ana_kaip = REG32(A2W_PLLD_ANA_KAIP),
    .ana_vco = REG32(A2W_PLLD_ANA_VCO),
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
    .cm_flock_bit = CM_LOCK_FLOCKH_BIT,
    .cm_pll = REG32(CM_PLLH),
    .ana_kaip = REG32(A2W_PLLH_ANA_KAIP),
    .ana_vco = REG32(A2W_PLLH_ANA_VCO),
  },
#endif
};

const struct pll_chan_def pll_chan_def[] = {
#if 0
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
#endif
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
    def->dig[2] = A2W_PASSWORD | (dig[2] & ~0xa8UL);
    def->dig[1] = A2W_PASSWORD | dig[1];
  } else {
    // set all HOLD bits
    *def->cm_pll = CM_PASSWORD | (*def->cm_pll | 0xaa);
    def->dig[3] = A2W_PASSWORD | dig[3];
    def->dig[2] = A2W_PASSWORD | (dig[2] & ~0x100401UL);
    def->dig[1] = A2W_PASSWORD | (dig[1] & ~0x4000UL);
  }
  def->dig[0] = A2W_PASSWORD | dig[0];

  *def->ctrl = A2W_PASSWORD | (*def->ctrl | A2W_PLL_CTRL_PRSTN);

  def->dig[3] = A2W_PASSWORD | (dig[3] | 0x42);
  def->dig[2] = A2W_PASSWORD | dig[2];
  def->dig[1] = A2W_PASSWORD | dig[1];
  def->dig[0] = A2W_PASSWORD | dig[0];
}

#ifdef RPI4
static uint32_t saved_ana_vco[PLL_NUM] = {
  [PLL_A] = 2,
  [PLL_B] = 2,
  [PLL_C] = 2,
  [PLL_D] = 2,
};
#endif

int set_pll_freq(enum pll pll, uint32_t freq) {
  const struct pll_def *def = &pll_def[pll];
  bool was_prescaled;
  bool is_prescaled;
  bool was_off;

  if (!freq) {
    *def->cm_pll = CM_PASSWORD | CM_PLL_ANARST;
    *def->ctrl = A2W_PASSWORD | A2W_PLL_CTRL_PWRDN;
    return 0;
  }

  was_off = !(*def->ctrl & A2W_PLL_CTRL_PRSTN);

#if RPI4
  uint32_t ana_vco = saved_ana_vco[pll];
  was_prescaled = BIT_SET(ana_vco, 4);
#else
  uint32_t ana3 = def->ana[3];
  uint32_t ana2 = def->ana[2];
  uint32_t ana1 = def->ana[1];
  uint32_t ana0 = def->ana[0];
  was_prescaled = BIT_SET(ana1, def->ana1_prescale_bit);
#endif

  // Divider is a fixed-point number with a 20-bit fractional part.
  uint32_t div = (((uint64_t)freq << 20) + xtal_freq / 2) / xtal_freq;

#if RPI4
  is_prescaled = freq > MHZ_TO_HZ(1600);
#else
  is_prescaled = freq > MHZ_TO_HZ(1750);
  if (is_prescaled) {
    div = (div + 1) >> 1;
    ana1 |= 1UL << def->ana1_prescale_bit;
  } else {
    ana1 &= ~(1UL << def->ana1_prescale_bit);
  }
#endif

  // Unmask the reference clock from the crystal oscillator.
  *REG32(A2W_XOSC_CTRL) |= A2W_PASSWORD | def->enable_bit;

#ifdef RPI4
  if (!was_off) {
    ana_vco |= 0x10000UL;
    *def->ana_vco = A2W_PASSWORD | ana_vco;
  }
#endif

#ifndef RPI4
  // PLLs with a prescaler will set all ANA registers, because there
  // is no other way to manipulate the prescaler enable bit.  Since
  // these registers also contain KA, KI and KP, their new values
  // must be set here, not through a write to A2W_PLLx_ANA_KAIP.
  if (pll == PLL_H) {
    ana0 = (ana0 & ~A2W_PLLH_ANA0_KA_MASK & ~A2W_PLLH_ANA0_KI_LO_MASK)
      | (2UL << A2W_PLLH_ANA0_KA_LSB)
      | (2UL << A2W_PLLH_ANA0_KI_LO_LSB);
    ana1 = (ana1 & ~A2W_PLLH_ANA1_KI_HI_MASK & ~A2W_PLLH_ANA1_KP_MASK)
      | (0UL << A2W_PLLH_ANA1_KI_HI_MASK)
      | (6UL << A2W_PLLH_ANA1_KP_LSB);
  } else {
    ana3 = (ana3 & ~A2W_PLL_ANA3_KA_MASK)
      | (2UL << A2W_PLL_ANA3_KA_LSB);
    ana1 = (ana1 & A2W_PLL_ANA1_KI_MASK & ~A2W_PLL_ANA1_KP_MASK)
      | (2UL << A2W_PLL_ANA1_KI_LSB)
      | (8UL << A2W_PLL_ANA1_KP_LSB);
  }
#endif

  // The Linux driver manipulates the prescaler bit before setting
  // a new NDIV if the prescaler is being turned _OFF_.
  // I believe it is a bug.
  // The programmable divider in a BCM283x apparently cannot cope
  // with frequencies above 1750 MHz. This is the reason for adding
  // an optional prescaler in the feedback path, so the maximum PLL
  // frequency can go up to 3.5 GHz, albeit at the cost of precision.
  // If the prescaler is turned off, the divider counter will be
  // exposed to the full VCO frequency (i.e. above 1750 MHz).
  // On BCM2711, bit 4 of ANA_VCO is manipulated after setting the
  // new divider if and only if the rate is going from a frequency
  // above 1.6 GHz to a frequency less than or equal 1.6 GHz.
  if (!was_prescaled || is_prescaled) {
#ifdef RPI4
    if (is_prescaled) {
      *def->ana_kaip = A2W_PASSWORD
	| (0 << A2W_PLL_ANA_KAIP_KA_LSB)
	| (2 << A2W_PLL_ANA_KAIP_KI_LSB)
	| (3 << A2W_PLL_ANA_KAIP_KP_LSB);
      ana_vco |= 0x10UL;
      *def->ana_vco = A2W_PASSWORD | ana_vco;
    } else {
      *def->ana_kaip = A2W_PASSWORD
	| (0 << A2W_PLL_ANA_KAIP_KA_LSB)
	| (2 << A2W_PLL_ANA_KAIP_KI_LSB)
	| (5 << A2W_PLL_ANA_KAIP_KP_LSB);
      ana_vco &= ~0x10UL;
      *def->ana_vco = A2W_PASSWORD | ana_vco;
    }
#else
    def->ana[3] = A2W_PASSWORD | ana3;
    def->ana[2] = A2W_PASSWORD | ana2;
    def->ana[1] = A2W_PASSWORD | ana1;
    def->ana[0] = A2W_PASSWORD | ana0;
#endif
  }

  // Set the PLL multiplier from the oscillator.
  *def->frac = A2W_PASSWORD | (div & A2W_PLL_FRAC_MASK);
  *def->ctrl = A2W_PASSWORD
    | (*def->ctrl
       & ~A2W_PLL_CTRL_PWRDN
       & ~A2W_PLL_CTRL_PDIV_MASK
       & ~def->ndiv_mask)
    | (div >> 20)
    | (1 << A2W_PLL_CTRL_PDIV_LSB);

  if (was_prescaled && !is_prescaled) {
#ifdef RPI4
    *def->ana_kaip = A2W_PASSWORD
      | (0 << A2W_PLL_ANA_KAIP_KA_LSB)
      | (2 << A2W_PLL_ANA_KAIP_KI_LSB)
      | (5 << A2W_PLL_ANA_KAIP_KP_LSB);
    ana_vco &= ~0x10UL;
    *def->ana_vco = A2W_PASSWORD | ana_vco;
#else
    def->ana[3] = A2W_PASSWORD | ana3;
    def->ana[2] = A2W_PASSWORD | ana2;
    def->ana[1] = A2W_PASSWORD | ana1;
    def->ana[0] = A2W_PASSWORD | ana0;
#endif
  }

  // Take the PLL out of reset.
  *def->cm_pll = CM_PASSWORD | (*def->cm_pll & ~CM_PLL_ANARST);

  // Wait for the PLL to lock.
  unsigned lockwait = PLL_MAX_LOCKWAIT;
  while (!BIT_SET(*REG32(CM_LOCK), def->cm_flock_bit))
    if (--lockwait == 0)
      break;

#ifdef RPI4
  if (!was_off) {
    ana_vco &= ~0x10000UL;
    *def->ana_vco = A2W_PASSWORD | ana_vco;
  }
  saved_ana_vco[pll] = ana_vco;
#endif

  if (!lockwait) {
    dprintf(INFO, "%s won't lock\n", def->name);
    return -1;
  }

  if (was_off)
    pll_start(pll);

  dprintf(SPEW, "%s frequency locked after %u iterations\n",
	  def->name, PLL_MAX_LOCKWAIT - lockwait);
  return 0;
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
  if ((orig_ctrl & A2W_PLL_CTRL_PRSTN) == 0)
    pll_start(PLL_B);

  *REG32(A2W_PLLB_ARM) = A2W_PASSWORD | 2;

  *REG32(CM_PLLB) = CM_PASSWORD | CM_PLL_DIGRST | CM_PLL_ANARST | CM_PLLB_HOLDARM_SET | CM_PLLB_LOADARM_SET;
  *REG32(CM_PLLB) = CM_PASSWORD | CM_PLL_DIGRST | CM_PLL_ANARST | CM_PLLB_HOLDARM_SET;
  *REG32(CM_PLLB) = CM_PASSWORD;

  *REG32(CM_ARMCTL) = CM_PASSWORD | 4 | CM_ARMCTL_ENAB_SET;
}

static int cmd_set_pll_freq(int argc, const cmd_args *argv) {
  if (argc != 3) {
    printf("usage: set_pll_freq <pll> <freq>\n");
    return -1;
  }
  enum pll pll = argv[1].u;
  uint32_t freq = argv[2].u;
  return set_pll_freq(pll, freq);
}

// in A2W_PLLC_CTRL
#define PDIV(n) ((n & 7) << 12)
// in A2W_PLLC_ANA1
#define KP(n) ((n & 0xf) << 15)
#define KI(n) ((n & 0x7) << 19)
// in A2W_PLLC_ANA3
#define KA(n) ((n & 7) << 7)
// for all but PLLH
#define ANA1_DOUBLE (1<<14)
// for PLLH
#define PLLH_ANA1_DOUBLE (1<<11)

void switch_vpu_to_src(int src) {
  *REG32(CM_VPUCTL) = CM_PASSWORD | (*REG32(CM_VPUCTL) & ~0xf) | (src & 0xf);
  while (*REG32(CM_VPUCTL) & CM_VPUCTL_BUSY_SET) {};
}

void setup_pllc(uint64_t target_freq) {
  int pdiv = 1;
  uint64_t xtal_in = xtal_freq;
  uint64_t goal_freq = target_freq / 2;
  uint64_t divisor = (goal_freq<<20) / xtal_in;
  int div = divisor >> 20;
  int frac = divisor & 0xfffff;
  printf("divisor 0x%llx -> %d+(%d/2^20)\n", divisor, div, frac);

  *REG32(CM_PLLC) = CM_PASSWORD | CM_PLLC_ANARST_SET;

  *REG32(A2W_XOSC_CTRL) |= A2W_PASSWORD | A2W_XOSC_CTRL_PLLCEN_SET;

  *REG32(A2W_PLLC_FRAC) = A2W_PASSWORD | frac;
  *REG32(A2W_PLLC_CTRL) = A2W_PASSWORD | div | PDIV(pdiv);
  printf("frac set to 0x%x\n", *REG32(A2W_PLLC_FRAC));

  *REG32(A2W_PLLC_ANA3) = A2W_PASSWORD | KA(2);
  *REG32(A2W_PLLC_ANA2) = A2W_PASSWORD | 0x0;
  *REG32(A2W_PLLC_ANA1) = A2W_PASSWORD | ANA1_DOUBLE | KI(2) | KP(8);
  *REG32(A2W_PLLC_ANA0) = A2W_PASSWORD | 0x0;

  *REG32(CM_PLLC) = CM_PASSWORD | CM_PLLC_DIGRST_SET;

  /* hold all */
  *REG32(CM_PLLC) = CM_PASSWORD | CM_PLLC_DIGRST_SET |
            CM_PLLC_HOLDPER_SET | CM_PLLC_HOLDCORE2_SET |
            CM_PLLC_HOLDCORE1_SET | CM_PLLC_HOLDCORE0_SET;

  *REG32(A2W_PLLC_DIG3) = A2W_PASSWORD | 0x0;
  *REG32(A2W_PLLC_DIG2) = A2W_PASSWORD | 0x400000;
  *REG32(A2W_PLLC_DIG1) = A2W_PASSWORD | 0x5;
  *REG32(A2W_PLLC_DIG0) = A2W_PASSWORD | div | 0x555000;

  *REG32(A2W_PLLC_CTRL) = A2W_PASSWORD | div | PDIV(pdiv) | A2W_PLLC_CTRL_PRSTN_SET;

  *REG32(A2W_PLLC_DIG3) = A2W_PASSWORD | 0x42;
  *REG32(A2W_PLLC_DIG2) = A2W_PASSWORD | 0x500401;
  *REG32(A2W_PLLC_DIG1) = A2W_PASSWORD | 0x4005;
  *REG32(A2W_PLLC_DIG0) = A2W_PASSWORD | div | 0x555000;

  *REG32(A2W_PLLC_CORE0) = A2W_PASSWORD | 4;

  *REG32(A2W_PLLC_PER) = A2W_PASSWORD | 4;

  *REG32(CM_PLLC) = CM_PASSWORD | CM_PLLC_DIGRST_SET |
            CM_PLLC_HOLDPER_SET | CM_PLLC_HOLDCORE2_SET |
            CM_PLLC_HOLDCORE1_SET | CM_PLLC_HOLDCORE0_SET | CM_PLLC_LOADCORE0_SET;

  *REG32(CM_PLLC) = CM_PASSWORD | CM_PLLC_DIGRST_SET |
            CM_PLLC_HOLDPER_SET | CM_PLLC_HOLDCORE2_SET |
            CM_PLLC_HOLDCORE1_SET | CM_PLLC_HOLDCORE0_SET;

  *REG32(CM_PLLC) = CM_PASSWORD | CM_PLLC_DIGRST_SET |
            CM_PLLC_HOLDCORE2_SET |
            CM_PLLC_HOLDCORE1_SET;

  while (!BIT_SET(*REG32(CM_LOCK), CM_LOCK_FLOCKC_BIT)) {}
}
