#include <app.h>
#include <dance.h>
#include <lib/tga.h>
#include <lk/console_cmd.h>
#include <lk/reg.h>
#include <platform/bcm28xx/hvs.h>
#include <platform/bcm28xx/pll.h>
#include <platform/bcm28xx/pll_read.h>
#include <platform/bcm28xx/power.h>
#include <platform/bcm28xx/pv.h>
#include <platform/bcm28xx/vec.h>
#include <stdio.h>

#include "pi-logo.h"

//extern uint8_t* pilogo;

enum vec_mode {
  ntsc,
  ntscj,
  pal,
  palm,
};

gfx_surface *framebuffer;
gfx_surface *logo;
int width;
int height;
int stride;

static void draw_background_grid(void) {
  //hvs_add_plane(framebuffer, 0, 0, false);
}

static void vec_init(const struct app_descriptor *app) {
  power_up_usb();
  hvs_initialize();
  *REG32(CM_VECDIV) = CM_PASSWORD | 4 << 12;
  *REG32(CM_VECCTL) = CM_PASSWORD | CM_SRC_PLLC_CORE0; // technically its on the PER tap
  *REG32(CM_VECCTL) = CM_PASSWORD | CM_VECCTL_ENAB_SET | CM_SRC_PLLC_CORE0;
  int rate = measure_clock(29);
  printf("vec rate: %f\n", ((float)rate)/1000/1000);

  *REG32(VEC_WSE_RESET) = 1;
  *REG32(VEC_SOFT_RESET) = 1;
  *REG32(VEC_WSE_CONTROL) = 0;

  *REG32(VEC_SCHPH) = 0x28;
  *REG32(VEC_CLMP0_START) = 0xac;
  *REG32(VEC_CLMP0_END) = 0xac;
  *REG32(VEC_CONFIG2) = VEC_CONFIG2_UV_DIG_DIS | VEC_CONFIG2_RGB_DIG_DIS;
  *REG32(VEC_CONFIG3) = VEC_CONFIG3_HORIZ_LEN_STD;
  *REG32(VEC_DAC_CONFIG) = VEC_DAC_CONFIG_DAC_CTRL(0xc) | VEC_DAC_CONFIG_DRIVER_CTRL(0xc) | VEC_DAC_CONFIG_LDO_BIAS_CTRL(0x46);
  *REG32(VEC_MASK0) = 0;
  enum vec_mode mode = ntsc;
  switch (mode) {
    case ntsc:
      *REG32(VEC_CONFIG0) = VEC_CONFIG0_NTSC_STD | VEC_CONFIG0_PDEN;
      *REG32(VEC_CONFIG1) = VEC_CONFIG1_C_CVBS_CVBS;
      break;
    case ntscj:
      *REG32(VEC_CONFIG0) = VEC_CONFIG0_NTSC_STD;
      *REG32(VEC_CONFIG1) = VEC_CONFIG1_C_CVBS_CVBS;
      break;
    case pal:
      *REG32(VEC_CONFIG0) = VEC_CONFIG0_PAL_BDGHI_STD;
      *REG32(VEC_CONFIG1) = VEC_CONFIG1_C_CVBS_CVBS;
      break;
    case palm:
      *REG32(VEC_CONFIG0) = VEC_CONFIG0_PAL_BDGHI_STD;
      *REG32(VEC_CONFIG1) = VEC_CONFIG1_C_CVBS_CVBS | VEC_CONFIG1_CUSTOM_FREQ;
      *REG32(VEC_FREQ3_2) = 0x223b;
      *REG32(VEC_FREQ1_0) = 0x61d1;
      break;
  }
  *REG32(VEC_DAC_MISC) = VEC_DAC_MISC_VID_ACT | VEC_DAC_MISC_DAC_RST_N;
  *REG32(VEC_CFG) = VEC_CFG_VEC_EN;
  struct pv_timings t;
  bool ntsc = true;
  if (ntsc) {
    t.vfp = 3;
    t.vsync = 4;
    t.vbp = 16;
    t.vactive = 240;

    t.vfp_even = 4;
    t.vsync_even = 3;
    t.vbp_even = 16;
    t.vactive_even = 240;

    t.interlaced = true;

    t.hfp = 14;
    t.hsync = 64;
    t.hbp = 60;
    t.hactive = 720;
  }
  setup_pixelvalve(&t, 2);
  if (!framebuffer) {
    width = t.hactive;
    height = t.vactive * 2;
    stride = t.hactive;
    framebuffer = gfx_create_surface(NULL, width, height, width, GFX_FORMAT_ARGB_8888);
  }
  int grid = 20;
  for (int x=0; x< width; x++) {
    for (int y=0; y < height; y++) {
      uint color = 0xff000000;
      if (y % grid == 0) color |= 0xffffff;
      if (y % grid == 1) color |= 0xffffff;
      if (x % grid == 0) color |= 0xffffff;
      if (x % grid == 1) color |= 0xffffff;
      gfx_putpixel(framebuffer, x, y, color);
    }
  }
  int list_start = display_slot;
  hvs_add_plane(framebuffer, 0, 0, false);
  hvs_terminate_list();
  *REG32(SCALER_DISPLIST1) = list_start;

  hvs_configure_channel(1, width, height, true);

  logo = tga_decode(pilogo, sizeof(pilogo), GFX_FORMAT_ARGB_8888);
  list_start = display_slot;
  hvs_add_plane(framebuffer, 0, 0, false);
  hvs_add_plane_scaled(logo, (width/2) - (logo->width/2), 0, 100, 100, false);
  hvs_terminate_list();
  *REG32(SCALER_DISPLIST1) = list_start;

  dance_start(logo, 1, &draw_background_grid);
}

static void vec_entry(const struct app_descriptor *app, void *args) {
}

APP_START(vec)
  .init = vec_init,
  .entry = vec_entry,
APP_END
