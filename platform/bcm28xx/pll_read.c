#include <stdio.h>
#include <platform/bcm28xx/pll_read.h>
#include <platform/bcm28xx/pll.h>
#include <platform/bcm28xx.h>
#include <lk/reg.h>
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
  // TODO, the optional /2 phase
  uint32_t freq = (xtal_freq * mult1) >> 20;
  return freq;
}

uint32_t plla() {
  return get_pll_freq(PLL_A);
}

uint32_t pllb() {
  return get_pll_freq(PLL_B);
}

uint32_t pllc() {
  return get_pll_freq(PLL_C);
}

uint32_t plld() {
  return get_pll_freq(PLL_D);
}

uint32_t pllh() {
  return get_pll_freq(PLL_H);
}

uint32_t pllc_core0(void) {
  uint32_t ctrl = *REG32(A2W_PLLC_CORE0);
  uint32_t div = ctrl & A2W_PLLC_CORE0_DIV_SET;
  uint32_t pllc_freq = pllc();
  return pllc_freq / div;
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
  case 4: // plla
    return 0;
  case 5: // pllc_core0
    return pllc_core0();
  case 6: // plld_per
    return 0;
  case 7: // pllh_aux
    return 0;
  case 8: // pllc_core1?
    return 0;
  case 9: // pllc_core2?
    return 0;
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

static void dump_plldiv_state(const char *prefix, uint32_t ctrl, uint32_t input) {
  uint32_t ctrl_val = *REG32(ctrl);
  dprintf(INFO, "\tA2W_%s: 0x%x\n", prefix, ctrl_val);
  uint8_t div = ctrl_val & 0xff;
  if (div == 0) return;
  uint32_t freq = input / div;
  dprintf(INFO, "\t%s freq: %u\n", prefix, freq);
}

static void dump_plldiv2_state(const char *prefix, uint32_t ctrl, uint32_t div) {
  uint32_t ctrl_val = *REG32(ctrl);
  uint32_t div_val = *REG32(div);
  dprintf(INFO, "CM_%sCTL: 0x%x\n", prefix, ctrl_val);
  dprintf(INFO, "CM_%sDIV: 0x%x\n", prefix, div_val);
}

static int cmd_pll_dump(int argc, const cmd_args *argv) {
  dump_pll_state(PLL_A);
  dump_pll_state(PLL_B);
  uint32_t pllc_freq = dump_pll_state(PLL_C);
  if (pllc_freq > 0) {
    dump_plldiv_state("PLLC_CORE0", A2W_PLLC_CORE0, pllc_freq);
    dump_plldiv_state("PLLC_CORE1", A2W_PLLC_CORE1, pllc_freq);
  }
  dump_pll_state(PLL_D);
  dump_pll_state(PLL_H);

  dump_plldiv2_state("VPU", CM_VPUCTL, CM_VPUDIV);
  return 0;
}
