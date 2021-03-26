#include <app.h>
#include <lk/reg.h>
#include <platform/bcm28xx/pll.h>
#include <platform/bcm28xx/power.h>
#include <platform/bcm28xx/udelay.h>
#include <stdio.h>
#include <string.h>

extern uint8_t arm_payload_start, arm_payload_end;

void mapBusToArm(uint32_t busAddr, uint32_t armAddr);
void setupClock(void);
void bridgeStart(bool cycleBrespBits);

#define ARM_CONTROL0  (ARM_BASE+0x000)
#define ARM_C0_SIZ1G     0x00000003
#define ARM_C0_BRESP1    0x00000004
#define ARM_C0_BRESP2    0x00000008
#define ARM_C0_FULLPERI  0x00000040
#define ARM_C0_APROTPASS  0x0000A000 // Translate 1:1
#define ARM_C0_APROTMSK  0x0000F000
#define ARM_TRANSLATE (ARM_BASE+0x100)
#define ARM_CONTROL1  (ARM_BASE+0x440)
#define ARM_C1_PERSON    0x00000100 // peripherals on
#define ARM_C1_REQSTOP   0x00000200 // ASYNC bridge request stop
#define ARM_ERRHALT   (ARM_BASE + 0x448)
#define ARM_ID        (ARM_BASE + 0x44C)

#define PM_PROC_ARMRSTN_CLR 0xffffffbf

static const uint8_t g_BrespTab[] = {
	0x10, 0x14, 0x10, 0x14, 0x10, 0x14, 0x10, 0x14, 0x10, 0x1C, 0x18, 0x1C, 0x18, 0x0,
	0x10, 0x14, 0x10, 0x1C, 0x10, 0x14, 0x10, 0x14, 0x10, 0x14, 0x10, 0x14, 0x10, 0x0,
	0x10, 0x14, 0x10, 0x1C, 0x18, 0x1C, 0x10, 0x14, 0x18, 0x1C, 0x10, 0x14, 0x10, 0x0,
	0x10, 0x14, 0x18, 0x14, 0x10, 0x14, 0x10, 0x14, 0x10, 0x14, 0x10, 0x14, 0x10, 0x0,
	0x10, 0x14, 0x18, 0x14, 0x18, 0x14, 0x10, 0x14, 0x10, 0x14, 0x10, 0x14, 0x18, 0x0
};

static void printregs(void) {
  printf("C0: 0x%x, C1: 0x%x, ERRHALT: 0x%x\n", *REG32(ARM_CONTROL0), *REG32(ARM_CONTROL1), *REG32(ARM_ERRHALT));
  printf("CM_LOCK: 0x%x\n", *REG32(CM_LOCK));
}

static void arm_init(const struct app_descriptor *app) {
  power_arm_start();
  printregs();
  printf("arm starting...\n");
  memcpy((void*)0xc0000000, &arm_payload_start, &arm_payload_end - &arm_payload_start);
  mapBusToArm(0xc0000000, 0);
  printf("armid 0x%x, C0 0x%x\n", *REG32(ARM_ID), *REG32(ARM_CONTROL0));
  /*
   * enable peripheral access, map arm secure bits to axi secure bits 1:1 and
   * set the mem size for who knows what reason.
   */
  *REG32(ARM_CONTROL0) |=
                  ARM_C0_BRESP2
                | ARM_C0_SIZ1G
                | ARM_C0_APROTPASS | ARM_C0_APROTMSK  // allow both kernel and userland to access mmio
                | ARM_C0_FULLPERI                     // allow access to all peripherals
                | (0x8 << 20)                         // ARM_C0_PRIO_PER
                | (0x5 << 24)                         // ARM_C0_PRIO_L2
                | (0xa << 28);                        // ARM_C0_PRIO_UC
  // | ARM_C0_AARCH64;
  *REG32(ARM_CONTROL1) |= ARM_C1_PERSON;

  printregs();

  setupClock();
  printregs();
  power_arm_start();
  printregs();
  bridgeStart(true);
  printregs();
}

// maps a 16mb chunk of ram
// the bus address has a resolution of 2mb
// the arm addr has a resolution of 16mb
// the entire mapping is 16mb long
// comments say there are 32 slots in the list (512mb mapped) an another 32 spare (1gig mapped)
void mapBusToArm(uint32_t busAddr, uint32_t armAddr) {
  volatile uint32_t* tte = (volatile uint32_t*)ARM_TRANSLATE;

  uint32_t index = armAddr >> 24; // div by 16mb
  uint32_t pte = busAddr >> 21; // div by 2mb
  printf("mapBusToArm index:%x, pte:%x\n", index, pte);

  tte[index] = pte;
}

void setupClock(void) {
  puts("initializing PLLB ...");

  /* oscillator->pllb */
  *REG32(A2W_XOSC_CTRL) |= A2W_PASSWORD | A2W_XOSC_CTRL_PLLBEN_SET;

  *REG32(A2W_PLLB_FRAC) = A2W_PASSWORD | 0xeaaa8; // out of 0x100000
  *REG32(A2W_PLLB_CTRL) = A2W_PASSWORD | 48 | 0x1000;

  *REG32(CM_PLLB) = CM_PASSWORD | CM_PLLB_DIGRST_SET | CM_PLLB_ANARST_SET;
  *REG32(CM_PLLB) = CM_PASSWORD | CM_PLLB_DIGRST_SET | CM_PLLB_ANARST_SET | CM_PLLB_HOLDARM_SET;

  *REG32(A2W_PLLB_ANA3) = A2W_PASSWORD | 0x100;
  *REG32(A2W_PLLB_ANA2) = A2W_PASSWORD | 0x0;
  *REG32(A2W_PLLB_ANA1) = A2W_PASSWORD | 0x140000;
  *REG32(A2W_PLLB_ANA0) = A2W_PASSWORD | 0x0;

  *REG32(CM_PLLB) = CM_PASSWORD | 0xAA; // hold all divider taps

  uint32_t dig0 = *REG32(A2W_PLLB_DIG0),
           dig1 = *REG32(A2W_PLLB_DIG1),
           dig2 = *REG32(A2W_PLLB_DIG2),
           dig3 = *REG32(A2W_PLLB_DIG3);

  *REG32(A2W_PLLB_DIG3) = A2W_PASSWORD | dig3;
  *REG32(A2W_PLLB_DIG2) = A2W_PASSWORD | (dig2 & 0xFFEFFBFE);
  *REG32(A2W_PLLB_DIG1) = A2W_PASSWORD | (dig1 & ~(1 << 14));
  *REG32(A2W_PLLB_DIG0) = A2W_PASSWORD | dig0;

  *REG32(A2W_PLLB_CTRL) |= A2W_PASSWORD | 0x20000;

  dig3 |= 0x42;

  *REG32(A2W_PLLB_DIG3) = A2W_PASSWORD | dig3;
  *REG32(A2W_PLLB_DIG2) = A2W_PASSWORD | dig2;
  *REG32(A2W_PLLB_DIG1) = A2W_PASSWORD | dig1;
  *REG32(A2W_PLLB_DIG0) = A2W_PASSWORD | dig0;


  *REG32(A2W_PLLB_ARM) = A2W_PASSWORD | 2;

  *REG32(CM_PLLB) = CM_PASSWORD | CM_PLLB_DIGRST_SET | CM_PLLB_ANARST_SET | CM_PLLB_HOLDARM_SET | CM_PLLB_LOADARM_SET;
  *REG32(CM_PLLB) = CM_PASSWORD | CM_PLLB_DIGRST_SET | CM_PLLB_ANARST_SET | CM_PLLB_HOLDARM_SET;
  *REG32(CM_PLLB) = CM_PASSWORD;

  *REG32(CM_ARMCTL) = CM_PASSWORD | 4 | CM_ARMCTL_ENAB_SET;

  printf("KAIP  = 0x%X\n", *REG32(A2W_PLLB_ANA_KAIP)); /* 0x228 */
  printf("MULTI = 0x%X\n", *REG32(A2W_PLLB_ANA_MULTI)); /* 0x613277 */

  puts("ARM clock succesfully initialized!");
}

void bridgeWriteBresp(uint8_t bits) {
  //printf("bits: 0x%x\n", bits);
  *REG32(ARM_CONTROL0) = (*REG32(ARM_CONTROL0) & ~(ARM_C0_BRESP1|ARM_C0_BRESP2)) | bits;
  udelay(30);
}

void bridgeCycleBresp(void) {
  puts("cycling through bresp bits ...");
  for (unsigned int i = 0; i < sizeof(g_BrespTab)/sizeof(g_BrespTab[0]); i++) {
    bridgeWriteBresp(g_BrespTab[i]);
  }
}

void bridgeStart(bool cycleBrespBits) {
  puts("setting up async bridge ...");

  if (cycleBrespBits) {
    *REG32(PM_PROC) |= PM_PASSWORD | ~PM_PROC_ARMRSTN_CLR;
    bridgeCycleBresp();
    *REG32(PM_PROC) |= PM_PASSWORD | ~PM_PROC_ARMRSTN_CLR;

    udelay(300);
  }

  puts("starting async bridge now!");
  *REG32(ARM_CONTROL1) &= ~ARM_C1_REQSTOP;
  udelay(300);

  if (!cycleBrespBits) {
    *REG32(PM_PROC) |= PM_PASSWORD | ~PM_PROC_ARMRSTN_CLR;
  }

  printf("bridge init done, PM_PROC is now: 0x%X!\n", *REG32(PM_PROC));
}

APP_START(arm)
  .init = arm_init,
//  .entry = arm_entry,
APP_END
