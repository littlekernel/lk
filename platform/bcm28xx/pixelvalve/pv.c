#include <lk/reg.h>
#include <platform/bcm28xx/pv.h>
#include <lk/console_cmd.h>
#include <platform/bcm28xx.h>

#define BV(b) (1 << b)

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

void setup_pixelvalve(struct pv_timings *t, int pvnr) {
  uint32_t addr;
  switch (pvnr) {
  case 0:
    addr = BCM_PERIPH_BASE_VIRT + 0x206000;
    break;
  default:
    return;
  }
  volatile struct pixel_valve *rawpv = addr;

  // reset the PV fifo
  rawpv->c = 0;
  rawpv->c = PV_CONTROL_FIFO_CLR | PV_CONTROL_EN;
  rawpv->c = 0;

  rawpv->horza = (t->hbp << 16) | t->hsync;
  rawpv->horzb = (t->hfp << 16) | t->hactive;

  rawpv->verta = (t->vbp << 16) | t->vsync;
  rawpv->vertb = (t->vfp << 16) | t->vactive;

#define CLK_SELECT(n) ((n & 3) << 2)
# define PV_CONTROL_CLK_SELECT_DSI              0
# define PV_CONTROL_CLK_SELECT_DPI_SMI_HDMI     1
# define PV_CONTROL_CLK_SELECT_VEC              2
#define PIXEL_REP(n) ((n & 0x3) << 4)
#define FIFO_LEVEL(n) ((n & 0x3f) << 15)

  rawpv->vc = BV(0) | // video enable
            BV(1); // continous

  rawpv->h_active = t->hactive;

  uint32_t fifo_len_bytes = 64;
  fifo_len_bytes = fifo_len_bytes - 3 * 6;

  rawpv->c = PV_CONTROL_EN |
            PV_CONTROL_FIFO_CLR |
            CLK_SELECT(PV_CONTROL_CLK_SELECT_DPI_SMI_HDMI) | // set to DPI clock
            PIXEL_REP(1 - 1) |
            BV(12) | // wait for h-start
            BV(13) | // trigger underflow
            BV(14) | // clear at start
            FIFO_LEVEL(fifo_len_bytes);
}
