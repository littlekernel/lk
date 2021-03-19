#pragma once

#include <platform/interrupts.h>

struct pv_timings {
  uint16_t vfp, vsync, vbp, vactive;
  uint16_t hfp, hsync, hbp, hactive;
  uint16_t vfp_even, vsync_even, vbp_even, vactive_even;
  bool interlaced;
  int clock_mux;
};

struct pixel_valve {
  volatile uint32_t c;
  volatile uint32_t vc;
  volatile uint32_t vsyncd_even;
  volatile uint32_t horza;
  volatile uint32_t horzb;
  volatile uint32_t verta;
  volatile uint32_t vertb;
  volatile uint32_t verta_even;
  volatile uint32_t vertb_even;
  volatile uint32_t int_enable;
  volatile uint32_t int_status;
  volatile uint32_t h_active;
};

void setup_pixelvalve(struct pv_timings *timings, int pvnr);
void setup_pv_interrupt(int pvnr, int_handler handler, void *arg);
struct pixel_valve *getPvAddr(int pvnr);

#define PV_CONTROL_FIFO_CLR (1<<1)
#define PV_CONTROL_EN       (1<<0)

#define PV_INTEN_HSYNC_START (1<<0)
#define PV_INTEN_HBP_START   (1<<1)
#define PV_INTEN_HACT_START  (1<<2)
#define PV_INTEN_HFP_START   (1<<3)
#define PV_INTEN_VSYNC_START (1<<4)
#define PV_INTEN_VBP_START   (1<<5)
#define PV_INTEN_VACT_START  (1<<6)
#define PV_INTEN_VFP_START   (1<<7)
#define PV_INTEN_VFP_END     (1<<8)
#define PV_INTEN_IDLE        (1<<9)
