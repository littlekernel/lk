#include <app.h>
#include <lib/tga.h>
#include <lk/console_cmd.h>
#include <lk/reg.h>
#include <pi-logo.h>
#include <platform/bcm28xx/hvs.h>
#include <platform/bcm28xx/pv.h>
#include <rand.h>
#include <stdio.h>

static int cmd_hvs_dance(int argc, const cmd_args *argv);

STATIC_COMMAND_START
STATIC_COMMAND("dance", "make the HVS dance in another direction", &cmd_hvs_dance)
STATIC_COMMAND_END(hvs_dance);

gfx_surface *fb;

struct item {
  int x, y;
  signed int xd, yd;
};

#define ITEMS 13
struct item items[ITEMS];

void do_frame_update(void) {

  int start = display_slot;
  for (int i=0; i<ITEMS; i++) {
    struct item *it = &items[i];
    if (it->x > (0x500 - fb->width)) it->xd *= -1;
    if (it->y > (0x400 - fb->height)) it->yd *= -1;
    it->x += it->xd;
    it->y += it->yd;

    if (it->x < 0) {
      it->x = it->x * -1;
      it->xd *= -1;
    }
    if (it->y < 0) {
      it->y = it->y * -1;
      it->yd *= -1;
    }
    hvs_add_plane(fb, it->x, it->y);
  }
  hvs_terminate_list();

  *REG32(SCALER_DISPLIST1) = start;

  if (display_slot > 4000) {
    display_slot = 0;
    puts("dlist loop");
  }
}

static enum handler_return pv_irq(void *pvnr) {
  struct pixel_valve *rawpv = getPvAddr(pvnr);
  uint32_t stat = rawpv->int_status;
  if (stat & PV_INTEN_VFP_START) {
    rawpv->int_status = PV_INTEN_VFP_START;
    struct hvs_channel *hvs_channels = (struct hvs_channel*)REG32(SCALER_DISPCTRL0);
    uint32_t stat1 = hvs_channels[1].dispstat;
    do_frame_update();
    printf("line: %d frame: %d start: %d\n", stat1 & 0xfff, (stat1 >> 12) & 0x3f, *REG32(SCALER_DISPLIST1));
  }
  return INT_NO_RESCHEDULE;
}

static int cmd_hvs_dance(int argc, const cmd_args *argv) {
  for (int i=0; i<ITEMS; i++) {
    struct item *it = &items[i];
    it->x = (unsigned int)rand() % 0x500;
    it->y = (unsigned int)rand() % 0x400;
    it->xd = rand() % 25;
    it->yd = rand() % 25;
    if (it->x > (0x500 - fb->width)) it->x = 0x500 - fb->width;
    if (it->y > (0x400 - fb->height)) it->y = 0x400 - fb->height;
  }
  return 0;
}

static void dance_init(struct app_descriptor *app) {
  fb = tga_decode(pilogo, sizeof(pilogo), GFX_FORMAT_ARGB_8888);
  if (!fb) {
    fb = gfx_create_surface(NULL, 10, 10, 10, GFX_FORMAT_ARGB_8888);
    for (unsigned int i=0; i<fb->width; i++) {
      int r = 0;
      if ((i%2) == 0) r = 255;
      for (unsigned int j=0; j<fb->height; j++) {
        int g = 0;
        if ((j%2) == 0) g = 255;
        gfx_putpixel(fb, i, j, 0xff000000 | (g << 8) | r);
      }
    }
  }
  fb->flush = 0;
}

static void dance_entry(const struct app_descriptor *app, void *args) {
  gfx_flush(fb);
  srand(*REG32(ST_CLO));
  for (int i=0; i<ITEMS; i++) {
    struct item *it = &items[i];
    it->x = (unsigned int)rand() % 0x500;
    it->y = (unsigned int)rand() % 0x400;
    it->xd = rand() % 25;
    it->yd = rand() % 25;
    if (it->x > (0x500 - fb->width)) it->x = 0x500 - fb->width;
    if (it->y > (0x400 - fb->height)) it->y = 0x400 - fb->height;
  }

  {
    puts("setting up pv interrupt");
    int pvnr = 2;
    struct pixel_valve *rawpv = getPvAddr(pvnr);
    rawpv->int_enable = 0;
    rawpv->int_status = 0xff;
    setup_pv_interrupt(pvnr, pv_irq, pvnr);
    rawpv->int_enable = PV_INTEN_VFP_START;
    puts("done");
  }
}

APP_START(hvs_dance)
  .init = dance_init,
  .entry = dance_entry,
APP_END
