#include <lk/console_cmd.h>
#include <lk/reg.h>
#include <stdio.h>

#define ARM_MEMORY_BASE 0xC0000000
#define VC4_PERIPH_BASE 0x7E000000
#define ARM_PERIPH_BASE 0x3F000000

#define ARM_BASE        0x7E00B000
#define ARM_CONTROL0    ARM_BASE+0x000
#define ARM_C0_SIZ1G        0x00000003
#define ARM_C0_FULLPERI     0x00000040
#define ARM_C0_APROTPASS    0x0000A000 // Translate 1:1
#define ARM_IRQ_ENBL3   ARM_BASE+0x218 // ARM irqs enable bits
#define ARM_IE_MAIL         0x00000002     // Mail IRQ
#define ARM_CONTROL1    ARM_BASE+0x440
#define ARM_C1_PERSON       0x00000100 // peripherals on
#define ARM_TRANSLATE   ARM_BASE+0x100
#define ARM_ID          ARM_BASE+0x44C

static int cmd_test_arm(int argc, const cmd_args *argv);

STATIC_COMMAND_START
STATIC_COMMAND("arm", "arm the arm!", &cmd_test_arm)
STATIC_COMMAND_END(arm);

// maps a 16mb chunk of ram
// the bus address has a resolution of 2mb
// the arm addr has a resolution of 16mb
// the entire mapping is 16mb long
// comments say there are 32 slots in the list (512mb mapped) an another 32 spare (1gig mapped)
void mapBusToArm(uint32_t busAddr, uint32_t armAddr) {
  volatile uint32_t* tte = REG32(ARM_TRANSLATE);

  uint32_t index = armAddr >> 24; // div by 16mb
  uint32_t pte = busAddr >> 21; // div by 2mb
  //printf("mapBusToArm index:%x, pte:%x\n", index, pte);

  tte[index] = pte;
}

static int cmd_test_arm(int argc, const cmd_args *argv) {
  for (uint32_t i = 0; i < 62; i++) {
    uint32_t offset = i * 0x1000000;
    mapBusToArm(ARM_MEMORY_BASE + offset, 0x0 + offset);
  }
  mapBusToArm(VC4_PERIPH_BASE, ARM_PERIPH_BASE);

  printf("ARM ID: 0x%X C0: 0x%X\n", *REG32(ARM_ID), *REG32(ARM_CONTROL0));

  /*
   * enable peripheral access, map arm secure bits to axi secure bits 1:1 and
   * set the mem size for who knows what reason.
   */
  *REG32(ARM_CONTROL0) |= 0x008 | ARM_C0_APROTPASS | ARM_C0_SIZ1G | ARM_C0_FULLPERI; // | ARM_C0_AARCH64;
  *REG32(ARM_CONTROL1) |= ARM_C1_PERSON;

  *REG32(ARM_IRQ_ENBL3) |= ARM_IE_MAIL;
  return 0;
}
