#include <app.h>
#include <kernel/timer.h>
#include <lk/console_cmd.h>
#include <lk/reg.h>
#include <platform/bcm28xx/hvs.h>
#include <rand.h>

static int cmd_hvs_dance(int argc, const cmd_args *argv);

STATIC_COMMAND_START
STATIC_COMMAND("dance", "make the HVS dance in another direction", &cmd_hvs_dance)
STATIC_COMMAND_END(hvs_dance);

timer_t updater;
gfx_surface *fb;
unsigned int x, y;
signed int xd, yd;
unsigned int width, height;

static enum handler_return updater_entry(struct timer *t, lk_time_t now, void *arg) {
  int start = display_slot;
  hvs_add_plane(fb, x, y);
  hvs_terminate_list();
  *REG32(SCALER_DISPLIST1) = start;

  if (x > (0x500 - width)) xd *= -1;
  if (y > (0x400 - height)) yd *= -1;

  x += xd;
  y += yd;

  if (display_slot > 200) display_slot = 0;
  return INT_NO_RESCHEDULE;
}

static int cmd_hvs_dance(int argc, const cmd_args *argv) {
  x = (unsigned int)rand() % 0x500;
  y = (unsigned int)rand() % 0x400;
  xd = rand() % 25;
  yd = rand() % 25;
  return 0;
}

static void dance_init(struct app_descriptor *app) {
  width = height = 10;
  fb = gfx_create_surface(NULL, width, height, width, GFX_FORMAT_ARGB_8888);
  fb->flush = 0;
  timer_initialize(&updater);
}

static void dance_entry(const struct app_descriptor *app, void *args) {
  for (unsigned int i=0; i<width; i++) {
    int r = 0;
    if ((i%2) == 0) r = 255;
    for (unsigned int j=0; j<height; j++) {
      int g = 0;
      if ((j%2) == 0) g = 255;
      gfx_putpixel(fb, i, j, 0xff000000 | (g << 8) | r);
    }
  }
  gfx_flush(fb);
  srand(*REG32(ST_CLO));
  x = (unsigned int)rand() % 0x500;
  y = (unsigned int)rand() % 0x400;
  xd = rand() % 25;
  yd = rand() % 25;
  timer_set_periodic(&updater, 20, updater_entry, NULL);
}

APP_START(hvs_dance)
  .init = dance_init,
  .entry = dance_entry,
APP_END
