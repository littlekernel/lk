/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/asm.h>
#include <arch/x86/descriptor.h>

#define MSR_EFER 0xc0000080
#define EFER_LME 0x00000100

#define PHYS_LOAD_ADDRESS (MEMBASE + KERNEL_LOAD_OFFSET)
#define PHYS_ADDR_DELTA (KERNEL_BASE + KERNEL_LOAD_OFFSET - PHYS_LOAD_ADDRESS)
#define PHYS(x) ((x) - PHYS_ADDR_DELTA)

.text
.code64

/* UEFI firmware entry point; AddressOfEntryPoint in the PE header points
 * here. Called with the MS x64 calling convention:
 *   rcx = EFI_HANDLE, rdx = EFI_SYSTEM_TABLE *
 * running in long mode on the firmware's page tables, at whatever address
 * the firmware loaded the image (everything below is position independent).
 * Shuffle the arguments into SysV registers and run the C stub; on error
 * it returns and we hand the status in rax back to the firmware.
 */
FUNCTION(efi_entry)
    /* rsi/rdi are callee-saved in the MS ABI but volatile in SysV; preserve
     * them across the call for the error-return path. The two pushes leave
     * rsp 16-byte aligned mod the MS entry state, re-bias for SysV. */
    push %rsi
    push %rdi
    sub  $8, %rsp

    mov  %rcx, %rdi
    mov  %rdx, %rsi
    /* Compute the runtime image base and size here rather than in C. The
     * compilers materialize the linker-provided symbols unpredictably in
     * this module (rip-relative yields the runtime address, but GOT
     * relaxation or constant folding yield the link-time address, and gcc
     * and clang disagree per symbol), while a PC32 lea and an absolute
     * immediate in asm are unambiguous. */
    lea  __code_start(%rip), %rdx
    mov  $__pe_size_of_image, %rcx
    call efi_main               /* does not return on success */

    add  $8, %rsp
    pop  %rdi
    pop  %rsi
    ret
END_FUNCTION(efi_entry)

/* void efi_handoff(uint64_t image_base) __NO_RETURN
 *
 * Called by the C stub after ExitBootServices succeeds, still on the
 * firmware's page tables (which identity map every range in the UEFI memory
 * map, including this code and the copy target). Copies the flat image to
 * its link-time physical address, drops out of long mode, and enters the
 * regular 32-bit boot path at real_start, exactly as a multiboot loader
 * would have (with no multiboot magic in eax).
 *
 * The firmware may have loaded the image above 4GB and its stack may live
 * above 4GB: this path only executes position-independent code until it
 * jumps into the copy (guaranteed < 4GB), and never touches the stack after
 * leaving long mode.
 */
FUNCTION(efi_handoff)
    cli

    /* clearing CR0.PG below faults if CR4.PCIDE is set; clear it first
     * (legal in long mode, the reverse is not) */
    mov  %cr4, %rax
    btr  $17, %rax              /* CR4.PCIDE */
    mov  %rax, %cr4

    /* copy the image to its link-time physical address. The source (rdi
     * argument) cannot overlap the target: AllocatePages(AllocateAddress)
     * succeeded there while the image was already loaded. */
    cld
    mov  %rdi, %rsi
    mov  $PHYS_LOAD_ADDRESS, %rdi
    mov  $__pe_size_of_image, %rcx
    rep movsb

    /* jump into the copy; PHYS() constants are valid from here on */
    mov  $PHYS(.Ltramp64), %rax
    jmp  *%rax

.Ltramp64:
    /* switch to the kernel GDT (the copy's physical alias); this must be
     * the 10-byte descriptor form, lgdt takes a 64-bit base in long mode */
    lgdt PHYS(_gdtr_phys64)

    /* far-return into the 32-bit code segment while still in long mode;
     * the firmware stack is still valid here */
    pushq $CODE_SELECTOR
    mov  $PHYS(.Ltramp32), %rax
    pushq %rax
    lretq

.code32
.Ltramp32:
    /* 32-bit compatibility mode; the stack is off limits from here on */
    movw $DATA_SELECTOR, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    /* turn paging off: drops the cpu out of long mode (EFER.LMA clears) */
    mov  %cr0, %eax
    btrl $31, %eax              /* CR0.PG */
    mov  %eax, %cr0

    /* scrub CR4 (drops LA57 if the firmware ran 5-level paging; the boot
     * path re-enables PAE itself) */
    xorl %eax, %eax
    mov  %eax, %cr4

    /* clear EFER.LME so the next paging enable is plain 32-bit until the
     * boot path sets up long mode again */
    movl $MSR_EFER, %ecx
    rdmsr
    andl $~EFER_LME, %eax
    wrmsr

    /* enter the regular boot path with no multiboot magic. rel32 jump:
     * the link-time displacement to real_start is correct at the physical
     * alias too, since both ends move together. */
    xorl %eax, %eax
    xorl %ebx, %ebx
    jmp  real_start
END_FUNCTION(efi_handoff)
