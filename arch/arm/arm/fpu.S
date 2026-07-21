/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/asm.h>

/*
 * fpexc is accessed via vmrs/vmsr's named-system-register form, which some
 * assemblers (clang's integrated assembler, and recent binutils) refuse to
 * recognize unless a VFP profile is in scope -- and arm_fpu_set_enable()
 * (below) is called unconditionally on boot/chainload regardless of whether
 * the target board actually has VFP hardware or passes an -mfpu compile
 * flag (see arch.c's chainload path), so the profile can't be relied on to
 * come from compile flags. Declare it explicitly here instead. This is safe
 * to do file-wide in a real .S file (unlike inline asm in a .c file, where
 * an embedded ".fpu" directive leaks into and can downgrade the profile for
 * unrelated inline asm later in the same translation unit): this file only
 * ever touches fpexc, so there's nothing else in scope for it to affect.
 */
.fpu vfp

.text

/* uint32_t arm_fpu_read_fpexc(void); */
FUNCTION(arm_fpu_read_fpexc)
    vmrs    r0, fpexc
    bx      lr
END_FUNCTION(arm_fpu_read_fpexc)

/* void arm_fpu_write_fpexc(uint32_t val); */
FUNCTION(arm_fpu_write_fpexc)
    vmsr    fpexc, r0
    bx      lr
END_FUNCTION(arm_fpu_write_fpexc)
