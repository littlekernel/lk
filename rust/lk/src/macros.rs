/*
 * Copyright (c) 2024 Google Inc. All rights reserved
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/// Helper macro for container_of and container_of_mut. Exported so it can be
/// used by those macros and not meant to be used directly.
#[macro_export]
macro_rules! container_of_const_or_mut {
    ($ptr:ident, $T:ty, $m:ident, $const_or_mut:ident) => {{
        // SAFETY: The caller must ensure that $ptr is a pointer to the $m
        // field in an object of type $T. This means that $ptr came from
        // addr_of!((*original_ptr).$m) so subtracting the offset of $m from
        // $ptr will restore the original pointer.
        let original_ptr = (($ptr).byte_sub(core::mem::offset_of!($T, $m)) as *$const_or_mut $T);

        // Check that type of $ptr matches type of $T.$m. This detects a
        // subclass of bugs at compile time where the wrong field or pointer
        // is passed and two types does not match.
        //
        // SAFETY: This should not generate any code.
        let _always_true = core::ptr::addr_of!((*original_ptr).$m) == $ptr;

        original_ptr
    }};
}

/// Get the pointer to a struct from a pointer to an embedded field.
/// Matches the C containerof define in include/shared/lk/macros.h.
/// Const version.
#[macro_export]
macro_rules! container_of {
    ($ptr:ident, $T:ty, $m:ident) => {
        $crate::container_of_const_or_mut!($ptr, $T, $m, const)
    };
}

/// Get the pointer to a struct from a pointer to an embedded field.
/// Matches the C containerof define in include/shared/lk/macros.h.
/// Mutable version.
///
/// To convert a pointer received by C code to a reference to a wrapping
/// rust struct a helper function could be used like so:
/// struct A {}
/// struct B {
///     a: A,
/// }
/// /// # SAFETY
/// ///
/// /// ptr_a must point to the a field in a B struct
/// unsafe fn ptr_a_to_ref_b<'a>(ptr_a: *mut A) -> &'a mut B {
///     unsafe { &mut *container_of_mut!(ptr_a, B, a) }
/// }
#[macro_export]
macro_rules! container_of_mut {
    ($ptr:ident, $T:ty, $m:ident) => {
        $crate::container_of_const_or_mut!($ptr, $T, $m, mut)
    };
}
