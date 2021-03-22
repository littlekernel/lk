#include <assert.h>
#include <lk/console_cmd.h>
#include <lk/reg.h>
#include <platform/bcm28xx.h>
#include <platform/bcm28xx/pv.h>

struct pixel_valve *getPvAddr(int pvnr) {
  uint32_t addr;
  assert(pvnr <= 2);
  switch (pvnr) {
  case 0:
    addr = BCM_PERIPH_BASE_VIRT + 0x206000;
    break;
  case 1:
    addr = BCM_PERIPH_BASE_VIRT + 0x207000;
    break;
  case 2:
    addr = BCM_PERIPH_BASE_VIRT + 0x807000;
    break;
  default:
    return NULL;
  }
  struct pixel_valve *rawpv = (struct pixel_valve*) addr;
  return rawpv;
}

unsigned int getPvIrq(int pvnr) {
  assert(pvnr <= 2);
  switch (pvnr) {
  case 0:
    return 45;
  case 1:
    return 46;
  case 2:
    return 42;
  default:
    return -1;
  }
}

void setup_pixelvalve(struct pv_timings *t, int pvnr) {
  struct pixel_valve *rawpv = getPvAddr(pvnr);

  // reset the PV fifo
  rawpv->c = 0;
  rawpv->c = PV_CONTROL_FIFO_CLR | PV_CONTROL_EN;
  rawpv->c = 0;

  rawpv->horza = (t->hbp << 16) | t->hsync;
  rawpv->horzb = (t->hfp << 16) | t->hactive;

  rawpv->verta = (t->vbp << 16) | t->vsync;
  rawpv->vertb = (t->vfp << 16) | t->vactive;

  if (t->interlaced) {
    rawpv->verta_even = (t->vbp_even << 16) | t->vsync_even;
    rawpv->vertb_even = (t->vfp << 16) | t->vactive_even;
  }

#define CLK_SELECT(n) ((n & 3) << 2)
# define PV_CONTROL_CLK_SELECT_DSI              0
# define PV_CONTROL_CLK_SELECT_DPI_SMI_HDMI     1
# define PV_CONTROL_CLK_SELECT_VEC              2
#define PIXEL_REP(n) (((n) & 0x3) << 4)
#define FIFO_LEVEL(n) ((n & 0x3f) << 15)

  rawpv->vc = BV(0) | // video enable
            BV(1)| // continous
            (t->interlaced ? BV(4) : 0);

  rawpv->h_active = t->hactive;

  uint32_t fifo_len_bytes = 64;
  fifo_len_bytes = fifo_len_bytes - 3 * 6;

  rawpv->c = PV_CONTROL_EN |
            PV_CONTROL_FIFO_CLR |
            //CLK_SELECT(PV_CONTROL_CLK_SELECT_DPI_SMI_HDMI) | // set to DPI clock
            CLK_SELECT(PV_CONTROL_CLK_SELECT_VEC) | // vec
            PIXEL_REP(1 - 1) |
            BV(12) | // wait for h-start
            BV(13) | // trigger underflow
            BV(14) | // clear at start
            FIFO_LEVEL(fifo_len_bytes);
}

void setup_pv_interrupt(int pvnr, int_handler handler, void *arg) {
  //struct pixel_valve *rawpv = getPvAddr(pvnr);
  unsigned int irq = getPvIrq(pvnr);

  register_int_handler(irq, handler, arg);
  unmask_interrupt(irq);
}
