#include <app.h>
#include <arch/ops.h>
#include <assert.h>
#include <kernel/timer.h>
#include <kernel/vm.h>
#include <lib/tga.h>
#include <lk/console_cmd.h>
#include <lk/err.h>
#include <lk/reg.h>
//#include <pi-logo.h>
#include <platform/bcm28xx/hvs.h>
#include <platform/bcm28xx/pv.h>
#include <platform/mailbox.h>
#include <rand.h>
#include <stdio.h>

static int cmd_hvs_dance(int argc, const cmd_args *argv);
static int cmd_hvs_limit(int argc, const cmd_args *argv);
static int cmd_hvs_delay(int argc, const cmd_args *argv);
static int cmd_dance_update(int argc, const cmd_args *argv);

STATIC_COMMAND_START
STATIC_COMMAND("dance", "make the HVS dance in another direction", &cmd_hvs_dance)
STATIC_COMMAND("l", "limit sprites", &cmd_hvs_limit)
STATIC_COMMAND("d", "delay updates", &cmd_hvs_delay)
STATIC_COMMAND("u", "update", &cmd_dance_update)
STATIC_COMMAND_END(hvs_dance);

gfx_surface *fb;

struct item {
  unsigned int x, y;
  signed int xd, yd;
};

#define ITEMS 580
struct item items[ITEMS];
uint32_t sprite_limit = 15;
int delay = 0;

uint32_t screen_width, screen_height;

#define SCALED

void do_frame_update(int frame) {
  if (delay != 0) {
    if ((frame % delay) != 0) return;
  }
  if ((display_slot + (sprite_limit * 7) + 1) > 4096) {
    printf("early dlist loop %d\n", display_slot);
    display_slot = 0;
  }

  int w,h;
#ifdef SCALED
  w = 100;
  h = 100;
#else
  w = fb->width;
  h = fb->height;
#endif

  int start = display_slot;
  for (unsigned int i=0; i < sprite_limit; i++) {
    struct item *it = &items[i];
    if (it->x > (screen_width - w)) it->xd *= -1;
    if (it->y > (screen_height - h)) it->yd *= -1;

    it->x += it->xd;
    it->y += it->yd;

#if 0
    if (it->x < 0) {
      it->x = it->x * -1;
      it->xd *= -1;
    }
    if (it->y < 0) {
      it->y = it->y * -1;
      it->yd *= -1;
    }
#endif
#ifdef SCALED
    hvs_add_plane_scaled(fb, it->x, it->y, w, h, false);
#else
    hvs_add_plane(fb, it->x, it->y, false);
#endif
  }
  hvs_terminate_list();

  if (display_slot > 4096) {
    //printf("dlist overflow!!!: %d\n", display_slot);
    display_slot = 0;
    do_frame_update(frame);
    return;
  }

  *REG32(SCALER_DISPLIST1) = start;

  if (display_slot > 4000) {
    display_slot = 0;
    //puts("dlist loop");
  }
}

static int cmd_dance_update(int argc, const cmd_args *argv) {
  printf("fb 0x%x is %dx%d\n", fb, fb->width, fb->height);
  do_frame_update(0);
  return 0;
}

uint32_t hsync, hbp, hact, hfp, vsync, vbp, vfps, last_vfps;

static enum handler_return pv_irq(void *pvnr) {
  uint32_t t = *REG32(ST_CLO);
  struct hvs_channel *hvs_channels = (struct hvs_channel*)REG32(SCALER_DISPCTRL0);
  struct pixel_valve *rawpv = getPvAddr((int)pvnr);
  uint32_t stat = rawpv->int_status;
  uint32_t ack = 0;
  uint32_t stat1 = hvs_channels[1].dispstat;
  if (stat & PV_INTEN_HSYNC_START) {
    ack |= PV_INTEN_HSYNC_START;
    hsync = t;
    if ((SCALER_STAT_LINE(stat1) % 5) == 0) {
      //hvs_channels[1].dispbkgnd = (1<<24) | 0x00ff00;
    } else {
      //hvs_channels[1].dispbkgnd = (1<<24) | 0xffffff;
    }
  }
  if (stat & PV_INTEN_HBP_START) {
    ack |= PV_INTEN_HBP_START;
    hbp = t;
  }
  if (stat & PV_INTEN_HACT_START) {
    ack |= PV_INTEN_HACT_START;
    hact = t;
  };
  if (stat & PV_INTEN_HFP_START) {
    ack |= PV_INTEN_HFP_START;
    hfp = t;
  }
  if (stat & PV_INTEN_VSYNC_START) {
    ack |= PV_INTEN_VSYNC_START;
    vsync = t;
  }
  if (stat & PV_INTEN_VBP_START) {
    ack |= PV_INTEN_VBP_START;
    vbp = t;
  }
  if (stat & PV_INTEN_VACT_START) {
    ack |= PV_INTEN_VACT_START;
  }
  if (stat & PV_INTEN_VFP_START) {
    ack |= PV_INTEN_VFP_START;
    last_vfps = vfps;
    vfps = t;
    //hvs_channels[1].dispbkgnd = (1<<24) | 0xff0000;
    do_frame_update((stat1 >> 12) & 0x3f);
    //printf("line: %d frame: %2d start: %4d ", stat1 & 0xfff, (stat1 >> 12) & 0x3f, *REG32(SCALER_DISPLIST1));
    //uint32_t idle = *REG32(SD_IDL);
    //uint32_t total = *REG32(SD_CYC);
    //*REG32(SD_IDL) = 0;
    //uint64_t idle_percent = ((uint64_t)idle * 100) / ((uint64_t)total);
    //printf("sdram usage: %d %d, %lld\n", idle, total, idle_percent);
    //printf("HSYNC:%5d HBP:%d HACT:%d HFP:%d VSYNC:%5d VBP:%5d VFPS:%d FRAME:%d\n", t - vsync, t-hbp, t-hact, t-hfp, t-vsync, t-vbp, t-vfps, t-last_vfps);
    //hvs_channels[1].dispbkgnd = (1<<24) | 0xffffff;
  }
  if (stat & PV_INTEN_VFP_END) {
    ack |= PV_INTEN_VFP_END;
  }
  if (stat & PV_INTEN_IDLE) {
    ack |= PV_INTEN_IDLE;
  }
  rawpv->int_status = ack;
  return INT_NO_RESCHEDULE;
}

static void dance_scramble(void) {
  int w,h;
#ifdef SCALED
  w = 100;
  h = 100;
#else
  w = fb->width;
  h = fb->height;
#endif
  for (int i=0; i<ITEMS; i++) {
    struct item *it = &items[i];
    it->x = (unsigned int)rand() % screen_width;
    it->y = (unsigned int)rand() % screen_height;
    it->xd = rand() % 10;
    it->yd = rand() % 10;
    if (it->x > (screen_width - w)) it->x = screen_width - fb->width;
    if (it->y > (screen_height - h)) it->y = screen_height - fb->height;
  }
}

static int cmd_hvs_dance(int argc, const cmd_args *argv) {
  dance_scramble();
  return 0;
}

static int cmd_hvs_limit(int argc, const cmd_args *argv) {
  if (argc == 2) sprite_limit = argv[1].u;
  if (sprite_limit > ITEMS) sprite_limit = ITEMS;
  return 0;
}

static int cmd_hvs_delay(int argc, const cmd_args *argv) {
  if (argc == 2) delay = argv[1].u;
  return 0;
}

void dance_start(gfx_surface* fbin, int hvs_channel) {
  fb = fbin;
  gfx_flush(fb);

  struct hvs_channel *hvs_channels = (struct hvs_channel*)REG32(SCALER_DISPCTRL0);
  hvs_channels[hvs_channel].dispbkgnd = (1<<24) | 0xffffff;
  screen_width = (hvs_channels[hvs_channel].dispctrl >> 12) & 0xfff;
  screen_height = (hvs_channels[hvs_channel].dispctrl & 0xfff) / 2;
  printf("detected a %dx%d screen\n", screen_width, screen_height);

  srand(*REG32(ST_CLO));
  dance_scramble();
  if (true) {
    puts("setting up pv interrupt");
    int pvnr = 2;
    struct pixel_valve *rawpv = getPvAddr(pvnr);
    rawpv->int_enable = 0;
    rawpv->int_status = 0xff;
    setup_pv_interrupt(pvnr, pv_irq, (void*)pvnr);
    rawpv->int_enable = PV_INTEN_VFP_START | 0x3f;
    //hvs_setup_irq();
    puts("done");
  }
}

static void dance_init(const struct app_descriptor *app) {
#if 0
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
#endif
  screen_width = 0x500;
  screen_height = 0x400;
}

static void dance_entry(const struct app_descriptor *app, void *args) {

}

APP_START(hvs_dance)
  .init = dance_init,
  .entry = dance_entry,
APP_END
