#include <stdio.h>
#include <platform/bcm28xx/pll_read.h>
#include <platform/bcm28xx/pll.h>
#include <platform/bcm28xx.h>
#include <lk/reg.h>
#include <lk/bits.h>
#include <lk/console_cmd.h>
#include <lk/debug.h>
#include <platform/bcm28xx/udelay.h>

uint32_t xtal_freq;

static int cmd_pll_dump(int argc, const cmd_args *argv);
static int cmd_measure_clock(int argc, const cmd_args *argv);
static int cmd_measure_clocks(int argc, const cmd_args *argv);

STATIC_COMMAND_START
STATIC_COMMAND("dump_pll_state", "print all pll state", &cmd_pll_dump)
STATIC_COMMAND("measure_clock", "measure an internal clock rate", &cmd_measure_clock)
STATIC_COMMAND("measure_clocks", "measure all internal clocks", &cmd_measure_clocks)
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
  uint32_t pdiv = (ctrl & A2W_PLL_CTRL_PDIV_MASK) >> A2W_PLL_CTRL_PDIV_LSB;
  if (pdiv == 0)
	  return 0;
  uint32_t frac = *def->frac & A2W_PLL_FRAC_MASK;
  uint32_t div = (ndiv << 20) | frac;
#ifndef RPI4
  if (BIT_SET(def->ana[1], def->ana1_prescale_bit))
    div <<= 1;
#endif
  uint64_t mult1 = (uint64_t)div * xtal_freq / pdiv;
  return mult1 >> 20;
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

// based on https://github.com/raspberrypi/linux/blob/rpi-4.19.y/drivers/clk/bcm/clk-bcm2835.c#L356
#define CM_KILL 0x40
#define CM_BUSY 0x80
#define CM_SRC_MASK 0xf
#define CM_SRC_BITS 4
#define CM_TCNT_SRC1_SHIFT 12

int measure_clock(int mux) {
  int divisor = 1000;
  *REG32(CM_TCNTCTL) = CM_PASSWORD | CM_KILL;
  *REG32(CM_TCNTCTL) = CM_PASSWORD | (mux & CM_SRC_MASK) | (mux >> CM_SRC_BITS) << CM_TCNT_SRC1_SHIFT;
  *REG32(CM_OSCCOUNT) = CM_PASSWORD | (xtal_freq / divisor);
  udelay(1);
  while (*REG32(CM_OSCCOUNT)) {
  }

  while (*REG32(CM_TCNTCTL) & CM_BUSY) {
  }

  int count = *REG32(CM_TCNTCNT);
  *REG32(CM_TCNTCNT) = CM_PASSWORD | 0;
  return count * divisor;
}

static int cmd_measure_clock(int argc, const cmd_args *argv) {
  if (argc != 2) {
    puts("error, missing argument");
                        // reg      offset default-freq when netbooting
    puts("1 - H264");   // CM_H264  0x28
    puts("2 - ISP");    // CM_ISP   0x30
    puts("3 - sdram");  // CM_SDC   0xa8
    puts("5 - VPU");    // CM_VPU   0x08  100,000khz
    puts("6 - OTP");    // CM_OTP   0x90  4,800khz
    puts("9 - ???");    //                500khz
    puts("12 - dsi0p"); // CM_DSI0P 0x60
    puts("13 - dsi1p"); // CM_DSI1P 0x160
    puts("14 - cam0");  // CM_CAM0  0x40
    puts("15 - cam1");  // CM_CAM1  0x48
    puts("17 - dpi");   // CM_DPI   0x68
    puts("18 - dsi0e"); // CM_DSI0E 0x58
    puts("19 - dsi1e"); // CM_DSI1E 0x158
    puts("20 - gp0");   // CM_GP0   0x70
    puts("21 - gp1");   // CM_GP1   0x78  25,000khz
    puts("22 - hsm");   // CM_HSM   0x88
    puts("23 - pcm");   // CM_PCM   0x98
    puts("24 - pwm");   // CM_PWM   0xa0
    puts("25 - slim");  // CM_SLIM  0xa8
    puts("27 - smi");   // CM_SMI   0xb0
    puts("28 - uart");  // CM_UART  0xf0  1,916khz
    puts("29 - vec");   // CM_VEC   0xf8
    puts("30 - ???");   //                44khz
    puts("38 - aveo");  // CM_AVEO  0x1b8
    puts("39 - emmc");  // CM_EMMC  0x1c0
    puts("42 - emmc2"); // CM_EMMC2 0x1d0
    return 0;
  }
  int mux = argv[1].u;
  int count = measure_clock(mux);
  printf("count is %d\n", count);
  return 0;
}

static int cmd_measure_clocks(int argc, const cmd_args *argv) {
  for (int i=0; i<43; i++) {
    int count = measure_clock(i);
    printf("clock #%d is %d\n", i, count);
  }
  return 0;
}
