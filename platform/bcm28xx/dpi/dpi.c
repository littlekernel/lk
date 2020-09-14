#include <dev/gpio.h>
#include <lk/console_cmd.h>
#include <lk/reg.h>
#include <platform/bcm28xx/dpi.h>
#include <platform/bcm28xx/gpio.h>
#include <platform/bcm28xx/hvs.h>
#include <platform/bcm28xx/pll_read.h>
#include <platform/bcm28xx/pv.h>
#include <stdio.h>
#include <stdlib.h>

static int cmd_dpi_start(int argc, const cmd_args *argv);

STATIC_COMMAND_START
STATIC_COMMAND("dpi_start", "start DPI interface", &cmd_dpi_start)
STATIC_COMMAND_END(dpi);

uint32_t *framebuffer;

static int cmd_dpi_start(int argc, const cmd_args *argv) {
  *REG32(SCALER_DISPCTRL) = SCALER_DISPCTRL_ENABLE;
  *REG32(SCALER_DISPCTRL0) = SCALER_DISPCTRLX_RESET;
  if (!framebuffer) {
    framebuffer = malloc(4*10*10);
  }
  for (int i=0; i< (10*10); i++) {
    framebuffer[i] = (i<<24) | (i << 16) | (i << 8) | i;
  }
  int list_start = display_slot;
  hvs_add_plane(framebuffer);
  hvs_terminate_list();

  *REG32(SCALER_DISPLIST0) = list_start;
  *REG32(SCALER_DISPLIST1) = list_start;
  *REG32(SCALER_DISPLIST2) = list_start;
  *REG32(SCALER_DISPCTRL0) = SCALER_DISPCTRLX_ENABLE | SCALER_DISPCTRL_W(10) | SCALER_DISPCTRL_H(10);
  *REG32(SCALER_DISPBKGND0) = SCALER_DISPBKGND_AUTOHS;

  // 0x200 means clock/2
  *REG32(CM_DPIDIV) = CM_PASSWORD | (0xf00 << 4);
  *REG32(CM_DPICTL) = CM_PASSWORD | CM_DPICTL_KILL_SET | CM_SRC_OSC;
  while (*REG32(CM_DPICTL) & CM_DPICTL_BUSY_SET) {};
  *REG32(CM_DPICTL) = CM_PASSWORD | CM_DPICTL_ENAB_SET | CM_SRC_OSC;
  while (*REG32(CM_DPICTL) & CM_DPICTL_BUSY_SET) {};
  printf("DPI clock set\n");
  int rate = measure_clock(17);
  printf("DPI clock measured at %d\n", rate);
  struct pv_timings t;
  t.vfp = 0;
  t.vsync = 1;
  t.vbp = 0;
  t.vactive = 10;

  t.hfp = 0;
  t.hsync = 1;
  t.hbp = 0;
  t.hactive = 10;

  setup_pixelvalve(&t, 0);

  *REG32(DPI_C) = DPI_ENABLE | DPI_OUTPUT_ENABLE_MODE;
  //gpio_config(0, kBCM2708Pinmux_ALT2); // pixel-clock
  gpio_config(2, kBCM2708Pinmux_ALT2); // vsync
  gpio_config(3, kBCM2708Pinmux_ALT2); // hsync
  gpio_config(5, kBCM2708Pinmux_ALT2); // D1
  return 0;
}
