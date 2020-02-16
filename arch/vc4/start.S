#include <lk/asm.h>

#define MANUAL_UART

.section .text.start
FUNCTION(_start)
.global _start
_start:
  di
  mov sp, _fstack
#ifdef MANUAL_UART
  bl manual_uart_cfg
#endif
  bl zero_bss
  bl lk_main
loop:
  b loop

.global core2_start
core2_start:
  mov r0, core2_stack_top
  ld sp, (r0)
  mov r0, 0
  mov r1, 'A'
  bl uart_putc
  bl core2_entry
loop2:
  b loop2

.global testit
testit:
  st r1, (r0)
  st r2, (r0+4)
  st r3, (r0+8)
  st r4, (r0+12)
  rts

#ifdef MANUAL_UART
manual_uart_cfg:
  mov r0, 0x7e200004 // GP_FSEL1
  mov r1, 0x00004000
  st r1, (r0)
  mov r0, 0x7e1010f4 // CM_UARTDIV
  //mov r1, 0x5a003900 19.2mhz divisor
  mov r1, 0x5a00a050 // 54mhz divisor
  st r1, (r0)
  mov r0, 0x7e1010f0 // CM_UARTCTL
  mov r1, 0x5a000211
  st r1, (r0)
  mov r0, 0x7e201044
  mov r1, 0x000007ff
  st r1, (r0)
  mov r0, 0x7e20102c
  mov r1, 0x00000070
  st r1, (r0)
  mov r0, 0x7e201024
  mov r1, 0x00000002
  st r1, (r0)
  mov r0, 0x7e201028
  mov r1, 0x0000003b
  st r1, (r0)
  mov r0, 0x7e201030
  mov r1, 0x00000301
  st r1, (r0)
  mov r0, 0x7e201000
  mov r1, 'U'
  st r1, (r0)
  rts
#endif
