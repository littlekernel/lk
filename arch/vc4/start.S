#include <lk/asm.h>

.section .text.start
FUNCTION(_start)
.global _start
_start:
  di
  mov sp, _fstack
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
