#include <lk/asm.h>
#include <arch/x86/descriptor.h>

#if WITH_SMP

#define LOAD_ADDRESS 0x4000
#define MSR_EFER 0xc0000080
#define EFER_LME 0x00000100

#define ARGS_ADDRESS (LOAD_ADDRESS + 0x1000)
#define ARGS_CR3     (ARGS_ADDRESS + 0x00)
#if ARCH_X86_64
#define ARGS_STACK   (ARGS_ADDRESS + 0x08)
#else
#define ARGS_STACK   (ARGS_ADDRESS + 0x04)
#endif

.text
.code16
// secondary cpu boot entry point and switch to protected mode
// enters with the following state:
// real mode, CS 0x0400, PC 0 (physical address 0x4000)
// LOAD_ADDRESS (physical) == mp_boot_start (virtual)
FUNCTION(mp_boot_start)
    // jump over the temp GDT below and switch to a flat memory segment (0)
    ljmp $0, $(LOAD_ADDRESS + (.Lafter_gdt - mp_boot_start))

.org 0x8
.Lgdt:
    // stuff the GDTR in the first entry
    .short (8*4)
    .int (LOAD_ADDRESS + 0x8) // address of .Lgdt
    .short 0

    // 0x8 code flat 32bit
    .short 0xffff           /* limit 15:00 */
    .short 0x0000           /* base 15:00 */
    .byte  0x00             /* base 23:16 */
    .byte  0b10011010       /* P(1) DPL(00) S(1) 1 C(0) R(1) A(0) */
    .byte  0b11001111       /* G(1) D(1) 0 0 limit 19:16 */
    .byte  0x0              /* base 31:24 */

    // 0x10 data flat 32bit
    .short 0xffff           /* limit 15:00 */
    .short 0x0000           /* base 15:00 */
    .byte  0x00             /* base 23:16 */
    .byte  0b10010010       /* P(1) DPL(00) S(1) 0 E(0) W(1) A(0) */
    .byte  0b11001111       /* G(1) B(1) 0 0 limit 19:16 */
    .byte  0x0              /* base 31:24 */

    // 0x18 code 64bit
    .short 0xffff           /* limit 15:00 */
    .short 0x0000           /* base 15:00 */
    .byte  0x00             /* base 23:16 */
    .byte  0b10011010       /* P(1) DPL(00) S(1) 1 C(0) R(1) A(0) */
    .byte  0b10101111       /* G(1) D(0) L(1) AVL(0) limit 19:16 */
    .byte  0x0              /* base 31:24 */

.Lafter_gdt:
    // load the above GDT
    lgdt (LOAD_ADDRESS + 0x08)

    // switch to protected mode
    movl  %cr0, %eax
    orl   $1, %eax
    movl  %eax, %cr0

    // jump to 32bit mode
    ljmpl $0x8, $(LOAD_ADDRESS + (.Lprot - mp_boot_start))
.Lprot:
    .code32
    // we're now in 32bit mode, set up the 32bit data segment registers
    mov   $0x10, %ax
    mov   %ax, %ss
    mov   %ax, %ds
    mov   %ax, %es
    mov   %ax, %fs
    mov   %ax, %gs

#if ARCH_X86_64
    // set up 64bit paging
    // set PAE bit in CR4
    mov %cr4, %eax
    or  $(1<<5), %eax
    mov %eax, %cr4

    // Enable Long mode
    movl $MSR_EFER ,%ecx
    rdmsr
    orl $EFER_LME,%eax
    wrmsr

    // load trampoline page table
    movl (ARGS_CR3), %eax
    mov %eax, %cr3

    // enable paging, now we're in 32bit compatibility mode
    mov %cr0,  %eax
    btsl $(31), %eax
    mov %eax,  %cr0

    // load a very temporary stack pointer
    movl $(LOAD_ADDRESS + 0x800), %esp

    // Use a far jump to get into 64bit mode
    pushl $0x18
    pushl $(LOAD_ADDRESS + (.Lfarjump64 - mp_boot_start))
    lret

.code64
.Lfarjump64:
    /* branch to our high address */
    movq  (.Lhigh_addr), %rax
    jmp  *%rax
.Lhigh_addr:
.quad mp_boot_start_high

#else //  ARCH_X86_32
    // set up 32bit paging

    // set PSE bit in CR4
    mov %cr4, %eax
    or  $(1<<4), %eax
    mov %eax, %cr4

    // load trampoline page table
    movl (ARGS_CR3), %eax
    mov %eax, %cr3

    // enable paging
    mov %cr0,  %eax
    btsl $(31), %eax
    mov %eax,  %cr0

    // Branch to the high address
    lea mp_boot_start_high, %eax
    jmp *%eax
#endif

DATA(mp_boot_end)
END_FUNCTION(mp_boot_start)

FUNCTION(mp_boot_start_high)
#if ARCH_X86_64
    // set up stack pointer
    mov (ARGS_STACK), %rsp

    // load the real GDT
    lgdt _gdtr

    push  $CODE_64_SELECTOR
    lea  .Lnext(%rip), %rax
    push %rax
    lretq
.Lnext:
    // zero out the segment registers
    xor %ax, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    // call into C
    cld
    mov $ARGS_ADDRESS, %rdi
    call secondary_entry
    jmp .

#else // ARCH_X86_32
    // set up stack pointer
    mov (ARGS_STACK), %esp

    // load the real GDT
    lgdt _gdtr

    push  $CODE_SELECTOR
    lea  .Lnext, %eax
    push %eax
    lret
.Lnext:

    // Load the real segment registers
    mov $DATA_SELECTOR, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    // call into C
    cld
    push $ARGS_ADDRESS
    call secondary_entry
    jmp .

#endif
END_FUNCTION(mp_boot_start_high)

#endif // WITH_SMP