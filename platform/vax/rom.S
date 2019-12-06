.text

.globl rom_putchar
.type rom_putchar@function
rom_putchar:
    .word 0x0004            # save r2
    movl  4(%ap), %r2
    jsb   *rom_putchar_addr
    ret

