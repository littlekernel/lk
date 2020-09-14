#pragma once

struct pv_timings {
  uint16_t vfp, vsync, vbp, vactive;
  uint16_t hfp, hsync, hbp, hactive;
};

void setup_pixelvalve(struct pv_timings *timings, int pvnr);
