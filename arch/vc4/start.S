#include <lk/asm.h>

.text
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
