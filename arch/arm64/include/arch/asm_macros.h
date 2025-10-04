/*
 * Copyright (c) 2014 Travis Geiselbrecht
 * Copyright 2020 The Fuchsia Authors
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#ifdef __ASSEMBLER__ // clang-format off

#ifndef __has_feature
#define __has_feature(x) 0
#endif

.macro push ra, rb
stp \ra, \rb, [sp,#-16]!
.endm

.macro pop ra, rb
ldp \ra, \rb, [sp], #16
.endm

/// Fill a register with a wide integer literal.
///
/// This emits the one to four instructions required to fill a 64-bit
/// register with a given bit pattern.  It uses as few instructions as
/// suffice for the particular value.
///
/// Parameters
///
///   * reg
///     - Required: Output 64-bit register.
///
///   * literal
///     - Required: An integer expression that can be evaluated immediately
///     without relocation.
///
.macro movlit reg, literal
mov \reg, #((\literal) & 0xffff)
.ifne (((\literal) >> 16) & 0xffff)
movk \reg, #(((\literal) >> 16) & 0xffff), lsl #16
.endif
.ifne (((\literal) >> 32) & 0xffff)
movk \reg, #(((\literal) >> 32) & 0xffff), lsl #32
.endif
.ifne (((\literal) >> 48) & 0xffff)
movk \reg, #(((\literal) >> 48) & 0xffff), lsl #48
.endif
.endm  // movlit

/// Materialize a symbol (with optional addend) into a register.
///
/// This emits the `adr` instruction or two-instruction sequence required
/// to materialize the address of a global variable or function symbol.
///
/// Parameters
///
///   * reg
///     - Required: Output 64-bit register.
///
///   * symbol
///     - Required: A symbolic expression requiring at most PC-relative reloc.
///
.macro adr_global reg, symbol
#if __has_feature(hwaddress_sanitizer)
  adrp \reg, :pg_hi21_nc:\symbol
  movk \reg, #:prel_g3:\symbol+0x100000000
  add \reg, \reg, #:lo12:\symbol
#elif defined(__AARCH64_CMODEL_TINY__)
  adr \reg, \symbol
#else
  adrp \reg, \symbol
  add \reg, \reg, #:lo12:\symbol
#endif
.endm  // adr_global

/// Load a 64-bit fixed global symbol (with optional addend) into a register.
///
/// This emits the `ldr` instruction or two-instruction sequence required to
/// load a global variable.  If multiple words are required, it's more
/// efficient to use `adr_global` and then `ldp` than to repeat `ldr_global`
/// with related locations.
///
/// Parameters
///
///   * reg
///     - Required: Output 64-bit register.
///
///   * symbol
///     - Required: A symbolic expression requiring at most PC-relative reloc.
///
.macro ldr_global reg, symbol
#ifdef __AARCH64_CMODEL_TINY__
  ldr \reg, \symbol
#else
  adrp \reg, \symbol
  ldr \reg, [\reg, #:lo12:\symbol]
#endif
.endm  // adr_global

.macro tbzmask, reg, mask, label, shift=0
.if \shift >= 64
    .error "tbzmask: unsupported mask, \mask"
.elseif \mask == 1 << \shift
    tbz     \reg, #\shift, \label
.else
    tbzmask \reg, \mask, \label, "(\shift + 1)"
.endif
.endm

.macro tbnzmask, reg, mask, label, shift=0
.if \shift >= 64
    .error "tbnzmask: unsupported mask, \mask"
.elseif \mask == 1 << \shift
    tbnz     \reg, #\shift, \label
.else
    tbnzmask \reg, \mask, \label, "(\shift + 1)"
.endif
.endm

.macro calloc_bootmem_aligned, new_ptr, new_ptr_end, tmp, size_shift, phys_offset
.if \size_shift < 4
    .error "calloc_bootmem_aligned: Unsupported size_shift, \size_shift"
.endif

    /* load boot_alloc_end */
    adrp    \tmp, boot_alloc_end
    ldr     \new_ptr, [\tmp, #:lo12:boot_alloc_end]

    /* align to page */
.if \size_shift > 12
    add     \new_ptr, \new_ptr, #(1 << \size_shift)
    sub     \new_ptr, \new_ptr, #1
.else
    add     \new_ptr, \new_ptr, #(1 << \size_shift) - 1
.endif
    and     \new_ptr, \new_ptr, #~((1 << \size_shift) - 1)

    /* add one page and store boot_alloc_end */
    add     \new_ptr_end, \new_ptr, #(1 << \size_shift)
    str     \new_ptr_end, [\tmp, #:lo12:boot_alloc_end]

    /* clean and invalidate boot_alloc_end pointer */
    add     x0, \tmp, #:lo12:boot_alloc_end
    mov     x1, #8
    bl      arch_clean_invalidate_cache_range

    /* calculate virtual address */
    sub     \new_ptr, \new_ptr, \phys_offset
    sub     \new_ptr_end, \new_ptr_end, \phys_offset

    /* clean and invalidate new page */
    mov     x0, \new_ptr
    mov     x1, #(1 << \size_shift)
    bl      arch_clean_invalidate_cache_range

    /* clear page */
    mov     \tmp, \new_ptr
.Lcalloc_bootmem_aligned_clear_loop\@:
    stp     xzr, xzr, [\tmp], #16
    cmp     \tmp, \new_ptr_end
    b.lo    .Lcalloc_bootmem_aligned_clear_loop\@
.endm

/* Set fault handler for next instruction */
.macro set_fault_handler, handler
.Lfault_location\@:
.pushsection .rodata.fault_handler_table
    .quad    .Lfault_location\@
    .quad    \handler
.popsection
.endm

#endif  // __ASSEMBLER__
