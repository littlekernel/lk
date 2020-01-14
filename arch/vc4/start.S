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
