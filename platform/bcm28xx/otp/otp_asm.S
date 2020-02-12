// copied from https://github.com/ptesarik/vc4boot

A2W_BASE = 0x7e102000
A2W_XOSC0         = 0x090

OTP_BASE = 0x7e20f000
OTP_CONFIG_REG          = 0x04
OTP_CTRL_LO_REG         = 0x08
OTP_CTRL_HI_REG         = 0x0c
OTP_STATUS_REG          = 0x10
OTP_DATA_REG            = 0x18
OTP_ADDR_REG            = 0x1c

#if RPI4
  OTP_READY_BIT = 1
#else
  OTP_READY_BIT = 0
#endif

.text
.global otp_read_internal
otp_read_internal:
  stm r6-r7,lr,(--sp)

  mov r6, r0 // save arg0 in r6
  bl otp_open
  mov r0, r6
  bl otp_read_reg
  mov r7, r0 // set the result asside so close wont clobber
  bl otp_close
  mov r0, r7

  ldm r6-r7,pc,(sp++)

otp_open:
  stm lr, (--sp)
  mov r2, 0x03
  mov r1, OTP_BASE
  mov r3, A2W_BASE + A2W_XOSC0
  st r2, (r1 + OTP_CONFIG_REG) // OTP_CONFIG_REG = 0x3

  mov r0, 2
1:
  ld r2, (r3) // read A2W_XOSC0
  addcmpbge r0, -1, 0, 1b // repeat 2 times total

  mov r2, 0
  st r2, (r1 + OTP_CTRL_HI_REG) // OTP_CTRL_HI_REG = 0
  st r2, (r1 + OTP_CTRL_LO_REG) // OTP_CTRL_LO_REG = 0

  mov r0, 2
1:
  ld r2, (r3) // read A2W_XOSC0
  addcmpbge r0, -1, 0, 1b // repeat 2 times total

  mov r2, 0x2
  st r2, (r1 + OTP_CONFIG_REG) // OTP_CONFIG_REG = 2
  ldm pc, (sp++)

otp_read_reg:
  stm lr, (--sp)
  mov r1, OTP_BASE
  mov r3, A2W_BASE + A2W_XOSC0
  st r0, (r1 + OTP_ADDR_REG) // OTP_ADDR_REG = arg0

  mov r0, 2
1:
  ld r2, (r3)
  addcmpbge r0, -1, 0, 1b // repeat 2 times total

  mov r2, 0
  ld r0, (r1 + OTP_ADDR_REG) // r0 = OTP_ADDR_REG
  st r2, (r1 + OTP_CTRL_HI_REG) // OTP_CTRL_HI_REG = 0
  st r2, (r1 + OTP_CTRL_LO_REG) // OTP_CTRL_LO_REG = 0

  mov r0, 2
1:
  ld r2, (r3)
  addcmpbge r0, -1, 0, 1b // repeat 2 times total

  mov r2, 1
  ld r0, (r1 + OTP_CTRL_LO_REG) // read ctrl low
  st r2, (r1 + OTP_CTRL_LO_REG) // ctrl low = 1
  ld r0, (r1 + OTP_CTRL_LO_REG) // read ctrl low again
1:
  mov r0, 2
2:
  ld r2, (r3)
  addcmpbge r0, -1, 0, 2b // repeat 2 times total

  ld r0, (r1 + OTP_STATUS_REG)  // read status reg
  btest r0, OTP_READY_BIT       // check if its ready
  beq 1b                        // if not ready, stall some more and try again
  ld r0, (r1 + OTP_DATA_REG)    // read final answer
  ldm pc, (sp++)

otp_close:
  stm lr, (--sp)
  mov r2, 0
  mov r1, OTP_BASE
  mov r3, A2W_BASE + A2W_XOSC0
  st r2, (r1 + OTP_CTRL_HI_REG) // OTP_CTRL_HI_REG = 0
  st r2, (r1 + OTP_CTRL_LO_REG) // OTP_CTRL_LO_REG = 0

  mov r0, 2
1:
  ld r2, (r3)
  addcmpbge r0, -1, 0, 1b // repeat 2 times total

  mov r2, 0
  st r2, (r1 + OTP_CONFIG_REG)
  ldm pc, (sp++)
