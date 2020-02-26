#include <stdio.h>
#include <platform/bcm28xx/pll_read.h>
#include <platform/bcm28xx/pll.h>
#include <platform/bcm28xx.h>
#include <lk/reg.h>
#include <lk/bits.h>
#include <lk/console_cmd.h>
#include <lk/debug.h>

uint32_t xtal_freq;

static int cmd_pll_dump(int argc, const cmd_args *argv);

STATIC_COMMAND_START
STATIC_COMMAND("dump_pll_state", "print all pll state", &cmd_pll_dump)
STATIC_COMMAND_END(pll);

uint32_t get_vpu_per_freq(void) {
  return clk_get_freq(CM_VPUDIV, CM_VPUCTL);
}

uint32_t get_uart_base_freq() {
  return clk_get_freq(CM_UARTDIV, CM_UARTCTL);
}

uint32_t get_pll_freq(enum pll pll) {
  const struct pll_def *def = &pll_def[pll];
  uint32_t ctrl = *def->ctrl;
  uint32_t ndiv = ctrl & def->ndiv_mask;
  uint32_t pdiv = (ctrl & def->pdiv_mask) >> def->pdiv_shift;
  uint32_t frac = *def->frac & A2W_PLL_FRAC_MASK;
  uint64_t mult1 = (ndiv << 20) | frac;
  mult1 *= pdiv;
  uint32_t freq = (xtal_freq * mult1) >> 20;
  if (BIT_SET(def->ana[1], def->ana1_pdiv_bit))
    freq >>= 1;
  return freq;
}

uint32_t get_pll_chan_freq(enum pll_chan chan) {
  const struct pll_chan_def *def = &pll_chan_def[chan];
  uint32_t ctrl_val = *def->ctrl;
  uint32_t div = ctrl_val & def->div_mask;
  if (BIT_SET(ctrl_val, def->chenb_bit) || div == 0)
    return 0;
  return get_pll_freq(def->pll) / div;
}

uint32_t clk_get_freq(uint32_t divreg, uint32_t ctlreg) {
  uint32_t div = *REG32(divreg);
  if (div == 0) return 0;
  uint64_t input_freq = clk_get_input_freq(ctlreg);
  return ((input_freq << 12) / div);
}

uint32_t clk_get_input_freq(uint32_t ctlreg) {
  uint32_t ctl = *REG32(ctlreg);
  switch (ctl & 0xf) {
  case 0: // GND clock source
    return 0;
  case 1: // crystal oscilator
    return xtal_freq;
  case 2: // test debug 0
  case 3: // test debug 1
    return 0;
  case 4: // plla_core
    return get_pll_chan_freq(PLL_CHAN_ACORE);
  case 5: // pllc_core0
    return get_pll_chan_freq(PLL_CHAN_CCORE0);
  case 6: // plld_per
    return get_pll_chan_freq(PLL_CHAN_DPER);
  case 7: // pllh_aux
    return get_pll_chan_freq(PLL_CHAN_HAUX);
  case 8: // pllc_core1
    return get_pll_chan_freq(PLL_CHAN_CCORE1);
  case 9: // pllc_core2
    return get_pll_chan_freq(PLL_CHAN_CCORE2);
  default:
    return 0;
  }
}

static uint32_t dump_pll_state(enum pll pll) {
  const struct pll_def *def = &pll_def[pll];
  uint32_t ctrl_val = *def->ctrl;
  uint32_t frac_value = *def->frac;
  dprintf(INFO, "A2W_%s_CTRL: 0x%x\n", def->name, ctrl_val);
  dprintf(INFO, "A2W_%s_FRAC: 0x%x\n", def->name, frac_value);
  uint32_t freq = get_pll_freq(pll);
  dprintf(INFO, "%s freq: %u\n", def->name, freq);
  return freq;
}

static void dump_pll_chan_state(enum pll_chan pll_chan) {
  const struct pll_chan_def *def = &pll_chan_def[pll_chan];
  uint32_t ctrl_val = *def->ctrl;
  dprintf(INFO, "\tA2W_%s: 0x%x\n", def->name, ctrl_val);
  uint32_t freq = get_pll_chan_freq(pll_chan);
  dprintf(INFO, "\t%s freq: %u\n", def->name, freq);
}

static void dump_plldiv2_state(const char *prefix, uint32_t ctrl, uint32_t div) {
  uint32_t ctrl_val = *REG32(ctrl);
  uint32_t div_val = *REG32(div);
  dprintf(INFO, "CM_%sCTL: 0x%x\n", prefix, ctrl_val);
  dprintf(INFO, "CM_%sDIV: 0x%x\n", prefix, div_val);
}

static int cmd_pll_dump(int argc, const cmd_args *argv) {
  enum pll pll;
  for (pll = 0; pll < PLL_NUM; ++pll) {
    uint32_t freq = dump_pll_state(pll);
    if (freq > 0) {
      enum pll_chan pll_chan;
      for (pll_chan = 0; pll_chan < PLL_CHAN_NUM; ++pll_chan)
	if (pll_chan_def[pll_chan].pll == pll)
	  dump_pll_chan_state(pll_chan);
    }
  }

  dump_plldiv2_state("VPU", CM_VPUCTL, CM_VPUDIV);
  return 0;
}
