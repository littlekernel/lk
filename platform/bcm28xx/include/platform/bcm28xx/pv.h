#pragma once

struct pv_timings {
  uint16_t vfp, vsync, vbp, vactive;
  uint16_t hfp, hsync, hbp, hactive;
};

void setup_pixelvalve(struct pv_timings *timings, int pvnr);

#define PV_CONTROL_FIFO_CLR (1<<1)
#define PV_CONTROL_EN       (1<<0)
