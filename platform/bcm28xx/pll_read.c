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

uint32_t compute_pll_freq(uint32_t ctrl, uint32_t frac) {
  // FIXME, ignores the addr passed in
  uint32_t ndiv = *REG32(ctrl) & A2W_PLLC_CTRL_NDIV_SET;
  uint32_t pdiv = (*REG32(ctrl) & A2W_PLLC_CTRL_PDIV_SET) >> A2W_PLLC_CTRL_PDIV_LSB;
  uint64_t mult1 = (ndiv << 20) | *REG32(frac);
  mult1 *= pdiv;
  // TODO, the optional /2 phase
  uint32_t freq = (xtal_freq * mult1) >> 20;
  return freq;
}

uint32_t plla() {
  return compute_pll_freq(*REG32(A2W_PLLA_CTRL), *REG32(A2W_PLLA_FRAC) & A2W_PLLA_FRAC_MASK);
}

uint32_t pllb() {
  return compute_pll_freq(*REG32(A2W_PLLB_CTRL), *REG32(A2W_PLLB_FRAC) & A2W_PLLB_FRAC_MASK);
}

uint32_t pllc() {
  //uint32_t ana1 = A2W_PLLC_ANA1;
  uint32_t ctrl = *REG32(A2W_PLLC_CTRL);
  uint32_t frac = *REG32(A2W_PLLC_FRAC) & A2W_PLLC_FRAC_MASK;
  return compute_pll_freq(ctrl, frac);
}

uint32_t plld() {
  return compute_pll_freq(*REG32(A2W_PLLD_CTRL), *REG32(A2W_PLLD_FRAC) & A2W_PLLD_FRAC_MASK);
}

uint32_t pllh() {
  return compute_pll_freq(*REG32(A2W_PLLH_CTRL), *REG32(A2W_PLLH_FRAC) & A2W_PLLH_FRAC_MASK);
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

static uint32_t dump_pll_state(const char *prefix, uint32_t ctrl, uint32_t frac) {
  uint32_t ctrl_val = *REG32(ctrl);
  uint32_t frac_value = *REG32(frac);
  dprintf(INFO, "A2W_%s_CTRL: 0x%x\n", prefix, ctrl_val);
  dprintf(INFO, "A2W_%s_FRAC: 0x%x\n", prefix, frac_value);
  uint32_t freq = compute_pll_freq(ctrl, frac);
  dprintf(INFO, "%s freq: %u\n", prefix, freq);
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
  dump_pll_state("PLLA", A2W_PLLA_CTRL, A2W_PLLA_FRAC);
  dump_pll_state("PLLB", A2W_PLLB_CTRL, A2W_PLLB_FRAC);
  uint32_t pllc_freq = dump_pll_state("PLLC", A2W_PLLC_CTRL, A2W_PLLC_FRAC);
  if (pllc_freq > 0) {
    dump_plldiv_state("PLLC_CORE0", A2W_PLLC_CORE0, pllc_freq);
    dump_plldiv_state("PLLC_CORE1", A2W_PLLC_CORE1, pllc_freq);
  }
  dump_pll_state("PLLD", A2W_PLLD_CTRL, A2W_PLLD_FRAC);
  dump_pll_state("PLLH", A2W_PLLH_CTRL, A2W_PLLH_FRAC);

  dump_plldiv2_state("VPU", CM_VPUCTL, CM_VPUDIV);
  return 0;
}
