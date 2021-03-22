#include <arch/ops.h>
#include <assert.h>
#include <lk/console_cmd.h>
#include <lk/reg.h>
#include <platform/bcm28xx/hvs.h>
#include <platform/interrupts.h>
#include <stdio.h>
#include <stdlib.h>

// note, 4096 slots total
volatile uint32_t* dlist_memory = REG32(SCALER_LIST_MEMORY);
volatile struct hvs_channel *hvs_channels = (volatile struct hvs_channel*)REG32(SCALER_DISPCTRL0);
int display_slot = 0;

static int cmd_hvs_dump(int argc, const cmd_args *argv);

STATIC_COMMAND_START
STATIC_COMMAND("hvs_dump", "dump hvs state", &cmd_hvs_dump)
STATIC_COMMAND_END(hvs);

static uint32_t gfx_to_hvs_pixel_format(gfx_format fmt) {
  switch (fmt) {
  case GFX_FORMAT_RGB_565:
    return HVS_PIXEL_FORMAT_RGB565;
  case GFX_FORMAT_RGB_332:
    return HVS_PIXEL_FORMAT_RGB332;
  case GFX_FORMAT_ARGB_8888:
  case GFX_FORMAT_RGB_x888:
    return HVS_PIXEL_FORMAT_RGBA8888;
  default:
    printf("warning, unsupported pixel format: %d\n", fmt);
    return 0;
  }
}

void hvs_add_plane(gfx_surface *fb, int x, int y, bool hflip) {
  assert(fb);
  printf("rendering FB of size %dx%d at %dx%d\n", fb->width, fb->height, x, y);
  dlist_memory[display_slot++] = CONTROL_VALID
    | CONTROL_WORDS(7)
    | CONTROL_PIXEL_ORDER(HVS_PIXEL_ORDER_ABGR)
//    | CONTROL0_VFLIP // makes the HVS addr count down instead, pointer word must be last line of image
    | (hflip ? CONTROL0_HFLIP : 0)
    | CONTROL_UNITY
    | CONTROL_FORMAT(gfx_to_hvs_pixel_format(fb->format));
  dlist_memory[display_slot++] = POS0_X(x) | POS0_Y(y) | POS0_ALPHA(0xff);
  dlist_memory[display_slot++] = POS2_H(fb->height) | POS2_W(fb->width);
  dlist_memory[display_slot++] = 0xDEADBEEF; // dummy for HVS state
  dlist_memory[display_slot++] = (uint32_t)fb->ptr | 0xc0000000;
  dlist_memory[display_slot++] = 0xDEADBEEF; // dummy for HVS state
  dlist_memory[display_slot++] = fb->stride * fb->pixelsize;
}

enum scaling_mode {
  none,
  PPF, // upscaling?
  TPZ // downscaling?
};

static void write_tpz(unsigned int source, unsigned int dest) {
  uint32_t scale = (1<<16) * source / dest;
  uint32_t recip = ~0 / scale;
  dlist_memory[display_slot++] = scale << 8;
  dlist_memory[display_slot++] = recip;
}

void hvs_add_plane_scaled(gfx_surface *fb, int x, int y, unsigned int width, unsigned int height, bool hflip) {
  assert(fb);
  //printf("rendering FB of size %dx%d at %dx%d, scaled down to %dx%d\n", fb->width, fb->height, x, y, width, height);
  enum scaling_mode xmode, ymode;
  if (fb->width > width) xmode = TPZ;
  else if (fb->width < width) xmode = PPF;
  else xmode = none;

  if (fb->height > height) ymode = TPZ;
  else if (fb->height < height) ymode = PPF;
  else ymode = none;

  int scl0;
  switch ((xmode << 2) | ymode) {
  case (PPF << 2) | PPF:
    scl0 = SCALER_CTL0_SCL_H_PPF_V_PPF;
    break;
  case (TPZ << 2) | PPF:
    scl0 = SCALER_CTL0_SCL_H_TPZ_V_PPF;
    break;
  case (PPF << 2) | TPZ:
    scl0 = SCALER_CTL0_SCL_H_PPF_V_TPZ;
    break;
  case (TPZ << 2) | TPZ:
    scl0 = SCALER_CTL0_SCL_H_TPZ_V_TPZ;
    break;
  default:
    puts("unsupported scale combonation");
  }

  int start = display_slot;
  dlist_memory[display_slot++] = 0 // CONTROL_VALID
//    | CONTROL_WORDS(14)
    | CONTROL_PIXEL_ORDER(HVS_PIXEL_ORDER_ABGR)
//    | CONTROL0_VFLIP // makes the HVS addr count down instead, pointer word must be last line of image
    | (hflip ? CONTROL0_HFLIP : 0)
    | CONTROL_FORMAT(gfx_to_hvs_pixel_format(fb->format))
    | (scl0 << 5)
    | (scl0 << 8); // SCL1
  dlist_memory[display_slot++] = POS0_X(x) | POS0_Y(y) | POS0_ALPHA(0xff);  // position word 0
  dlist_memory[display_slot++] = width | (height << 16);                    // position word 1
  dlist_memory[display_slot++] = POS2_H(fb->height) | POS2_W(fb->width);    // position word 2
  dlist_memory[display_slot++] = 0xDEADBEEF;                                // position word 3, dummy for HVS state
  dlist_memory[display_slot++] = (uint32_t)fb->ptr | 0xc0000000;            // pointer word 0
  dlist_memory[display_slot++] = 0xDEADBEEF;                                // pointer context word 0 dummy for HVS state
  dlist_memory[display_slot++] = fb->stride * fb->pixelsize;                // pitch word 0
  dlist_memory[display_slot++] = 0x100;    // LBM base addr

#if 0
  bool ppf = false;
  if (ppf) {
    uint32_t xscale = (1<<16) * fb->width / width;
    uint32_t yscale = (1<<16) * fb->height / height;

    dlist_memory[display_slot++] = SCALER_PPF_AGC | (xscale << 8);
    dlist_memory[display_slot++] = SCALER_PPF_AGC | (yscale << 8);
    dlist_memory[display_slot++] = 0xDEADBEEF; //scaling context
  }
#endif

  if (xmode == PPF) {
    puts("unfinished");
  }

  if (ymode == PPF) {
    puts("unfinished");
  }

  if (xmode == TPZ) {
    write_tpz(fb->width, width);
  }

  if (ymode == TPZ) {
    write_tpz(fb->height, height);
    dlist_memory[display_slot++] = 0xDEADBEEF; // context for scaling
  }

  //printf("entry size: %d, spans 0x%x-0x%x\n", display_slot - start, start, display_slot);
  dlist_memory[start] |= CONTROL_VALID | CONTROL_WORDS(display_slot - start);
}

void hvs_terminate_list(void) {
  dlist_memory[display_slot++] = CONTROL_END;
}

static enum handler_return hvs_irq(void *unused) {
  puts("hvs interrupt");
  return INT_NO_RESCHEDULE;
}

void hvs_initialize() {
  *REG32(SCALER_DISPCTRL) &= ~SCALER_DISPCTRL_ENABLE; // disable HVS
  *REG32(SCALER_DISPCTRL) = SCALER_DISPCTRL_ENABLE | 0x9a0dddff; // re-enable HVS
  for (int i=0; i<3; i++) {
    hvs_channels[i].dispctrl = SCALER_DISPCTRLX_RESET;
    hvs_channels[i].dispctrl = 0;
    hvs_channels[i].dispbkgnd = 0x1020202; // bit 24
  }

  hvs_channels[2].dispbase = BASE_BASE(0)      | BASE_TOP(0xf00);
  hvs_channels[1].dispbase = BASE_BASE(0xf10)  | BASE_TOP(0x4b00);
  hvs_channels[0].dispbase = BASE_BASE(0x4b10) | BASE_TOP(0x7700);

  hvs_wipe_displaylist();

  *REG32(SCALER_DISPEOLN) = 0x40000000;
}

void hvs_setup_irq() {
  register_int_handler(33, hvs_irq, NULL);
  unmask_interrupt(33);
}

void hvs_configure_channel(int channel, int width, int height) {
  hvs_channels[channel].dispctrl = SCALER_DISPCTRLX_RESET;
  hvs_channels[channel].dispctrl = SCALER_DISPCTRLX_ENABLE | SCALER_DISPCTRL_W(width) | SCALER_DISPCTRL_H(height);

  hvs_channels[channel].dispbkgnd = SCALER_DISPBKGND_AUTOHS | 0x020202 | SCALER_DISPBKGND_INTERLACE;
  // the SCALER_DISPBKGND_INTERLACE flag makes the HVS alternate between sending even and odd scanlines
}

void hvs_wipe_displaylist(void) {
  for (int i=0; i<1024; i++) {
    dlist_memory[i] = CONTROL_END;
  }
  display_slot = 0;
}

static bool bcm_host_is_model_pi4(void) {
  return false;
}

void hvs_print_position0(uint32_t w) {
  printf("position0: 0x%x\n", w);
  if (bcm_host_is_model_pi4()) {
    printf("    x: %d y: %d\n", w & 0x3fff, (w >> 16) & 0x3fff);
  } else {
    printf("    x: %d y: %d\n", w & 0xfff, (w >> 12) & 0xfff);
  }
}
void hvs_print_control2(uint32_t w) {
  printf("control2: 0x%x\n", w);
  printf("  alpha: 0x%x\n", (w >> 4) & 0xffff);
  printf("  alpha mode: %d\n", (w >> 30) & 0x3);
}
void hvs_print_word1(uint32_t w) {
  printf("  word1: 0x%x\n", w);
}
void hvs_print_position2(uint32_t w) {
  printf("position2: 0x%x\n", w);
  printf("  width: %d height: %d\n", w & 0xffff, (w >> 16) & 0xfff);
}
void hvs_print_position3(uint32_t w) {
  printf("position3: 0x%x\n", w);
}
void hvs_print_pointer0(uint32_t w) {
  printf("pointer word: 0x%x\n", w);
}
void hvs_print_pointerctx0(uint32_t w) {
  printf("pointer context word: 0x%x\n", w);
}
void hvs_print_pitch0(uint32_t w) {
  printf("pitch word: 0x%x\n", w);
}

static int cmd_hvs_dump(int argc, const cmd_args *argv) {
  printf("SCALER_DISPCTRL: 0x%x\n", *REG32(SCALER_DISPCTRL));
  printf("SCALER_DISPSTAT: 0x%x\n", *REG32(SCALER_DISPSTAT));
  printf("SCALER_DISPEOLN: 0x%08x\n", *REG32(SCALER_DISPEOLN));
  printf("SCALER_DISPLIST0: 0x%x\n", *REG32(SCALER_DISPLIST0));
  uint32_t list1 = *REG32(SCALER_DISPLIST1);
  printf("SCALER_DISPLIST1: 0x%x\n", list1);
  printf("SCALER_DISPLIST2: 0x%x\n\n", *REG32(SCALER_DISPLIST2));
  for (int i=0; i<3; i++) {
    printf("SCALER_DISPCTRL%d: 0x%x\n", i, hvs_channels[i].dispctrl);
    printf("  width: %d\n", (hvs_channels[i].dispctrl >> 12) & 0xfff);
    printf("  height: %d\n", hvs_channels[i].dispctrl & 0xfff);
    printf("SCALER_DISPBKGND%d: 0x%x\n", i, hvs_channels[i].dispbkgnd);
    uint32_t stat = hvs_channels[i].dispstat;
    printf("SCALER_DISPSTAT%d: 0x%x\n", i, stat);
    printf("mode: %d\n", (stat >> 30) & 0x3);
    if (stat & (1<<29)) puts("full");
    if (stat & (1<<28)) puts("empty");
    printf("unknown: 0x%x\n", (stat >> 18) & 0x3ff);
    printf("frame count: %d\n", (stat >> 12) & 0x3f);
    printf("line: %d\n", SCALER_STAT_LINE(stat));
    uint32_t base = hvs_channels[i].dispbase;
    printf("SCALER_DISPBASE%d: base 0x%x top 0x%x\n\n", i, base & 0xffff, base >> 16);
  }
  for (uint32_t i=list1; i<(list1+64); i++) {
    printf("dlist[%x]: 0x%x\n", i, dlist_memory[i]);
    if (dlist_memory[i] & BV(31)) {
      puts("(31)END");
      break;
    }
    if (dlist_memory[i] & BV(30)) {
      int x = i;
      int words = (dlist_memory[i] >> 24) & 0x3f;
      for (unsigned int index=i; index < (i+words); index++) {
        printf("raw dlist[%d] == 0x%x\n", index-i, dlist_memory[index]);
      }
      bool unity;
      printf("  (3:0)format: %d\n", dlist_memory[i] & 0xf);
      if (dlist_memory[i] & (1<<4)) puts("  (4)unity");
      printf("  (7:5)SCL0: %d\n", (dlist_memory[i] >> 5) & 0x7);
      printf("  (10:8)SCL1: %d\n", (dlist_memory[i] >> 8) & 0x7);
      if (false) { // is bcm2711
        if (dlist_memory[i] & (1<<11)) puts("  (11)rgb expand");
        if (dlist_memory[i] & (1<<12)) puts("  (12)alpha expand");
      } else {
        printf("  (12:11)rgb expand: %d\n", (dlist_memory[i] >> 11) & 0x3);
      }
      printf("  (14:13)pixel order: %d\n", (dlist_memory[i] >> 13) & 0x3);
      if (false) { // is bcm2711
        unity = dlist_memory[i] & (1<<15);
      } else {
        unity = dlist_memory[i] & (1<<4);
        if (dlist_memory[i] & (1<<15)) puts("  (15)vflip");
        if (dlist_memory[i] & (1<<16)) puts("  (16)hflip");
      }
      printf("  (18:17)key mode: %d\n", (dlist_memory[i] >> 17) & 0x3);
      if (dlist_memory[i] & (1<<19)) puts("  (19)alpha mask");
      printf("  (21:20)tiling mode: %d\n", (dlist_memory[i] >> 20) & 0x3);
      printf("  (29:24)words: %d\n", words);
      x++;
      hvs_print_position0(dlist_memory[x++]);
      if (bcm_host_is_model_pi4()) {
        hvs_print_control2(dlist_memory[x++]);
      }
      if (unity) {
        puts("unity scaling");
      } else {
        hvs_print_word1(dlist_memory[x++]);
      }
      hvs_print_position2(dlist_memory[x++]);
      hvs_print_position3(dlist_memory[x++]);
      hvs_print_pointer0(dlist_memory[x++]);
      hvs_print_pointerctx0(dlist_memory[x++]);
      hvs_print_pitch0(dlist_memory[x++]);
      if (words > 1) {
        i += words - 1;
      }
    }
  }
  return 0;
}

__WEAK status_t display_get_framebuffer(struct display_framebuffer *fb) {
  // TODO, have a base layer exposed via this
  return -1;
}
