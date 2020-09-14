#include <lk/console_cmd.h>
#include <lk/reg.h>
#include <platform/bcm28xx/hvs.h>
#include <stdio.h>

// note, 4096 slots total
volatile uint32_t* dlist_memory = REG32(SCALER_LIST_MEMORY);
int display_slot = 0;

static int cmd_hvs_dump(int argc, const cmd_args *argv);

STATIC_COMMAND_START
STATIC_COMMAND("hvs_dump", "dump hvs state", &cmd_hvs_dump)
STATIC_COMMAND_END(hvs);

void hvs_add_plane(void *framebuffer) {
  dlist_memory[display_slot++] = CONTROL_VALID
    | CONTROL_WORDS(7)
    | CONTROL_PIXEL_ORDER(HVS_PIXEL_ORDER_ARGB)
    | CONTROL_UNITY
    | CONTROL_FORMAT(HVS_PIXEL_FORMAT_RGB565);
  dlist_memory[display_slot++] = CONTROL0_X(0) | CONTROL0_Y(0);
  dlist_memory[display_slot++] = CONTROL2_H(10) | CONTROL2_W(10);
  dlist_memory[display_slot++] = 0xDEADBEEF; // dummy for HVS state
  dlist_memory[display_slot++] = framebuffer;
  dlist_memory[display_slot++] = 0xDEADBEEF; // dummy for HVS state
  dlist_memory[display_slot++] = 4 * 10; // stride
}

void hvs_terminate_list(void) {
  dlist_memory[display_slot++] = CONTROL_END;
}

static int cmd_hvs_dump(int argc, const cmd_args *argv) {
  printf("SCALER_DISPCTRL: 0x%x\n", *REG32(SCALER_DISPCTRL));
  printf("SCALER_DISPLIST0: 0x%x\n", *REG32(SCALER_DISPLIST0));
  printf("SCALER_DISPLIST1: 0x%x\n", *REG32(SCALER_DISPLIST1));
  printf("SCALER_DISPLIST2: 0x%x\n", *REG32(SCALER_DISPLIST2));
  printf("SCALER_DISPCTRL0: 0x%x\n", *REG32(SCALER_DISPCTRL0));
  printf("SCALER_DISPBKGND0: 0x%x\n", *REG32(SCALER_DISPBKGND0));
  printf("SCALER_DISPSTAT0: 0x%x\n", *REG32(SCALER_DISPSTAT0));
  for (int i=0; i<16; i++) {
    printf("dlist[%d]: 0x%x\n", i, dlist_memory[i]);
  }
  return 0;
}
