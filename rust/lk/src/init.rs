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

pub use crate::sys::lk_init_flags;
pub use crate::sys::lk_init_level;
pub use crate::sys::lk_init_struct;

// Note: Moved sys to lk_sys crate. As such, these impls have also moved to that crate.

#[macro_export]
macro_rules! LK_INIT_HOOK_FLAGS {
    ($name:ident, $hook:expr, $level:expr, $flags:expr) => {
        #[unsafe(link_section = "lk_init")]
        // #[unsafe(no_mangle)]
        #[used]
        static $name: $crate::init::lk_init_struct = $crate::init::lk_init_struct::new(
            $level,
            $flags,
            $hook,
            (concat!(stringify!($name), "\0").as_bytes()).as_ptr().cast(),
        );
    };
}

#[macro_export]
macro_rules! LK_INIT_HOOK {
    ($name:ident, $hook:expr, $level:expr) => {
        $crate::LK_INIT_HOOK_FLAGS!(
            $name,
            $hook,
            $level,
            $crate::init::lk_init_flags::LK_INIT_FLAG_PRIMARY_CPU
        );
    };
}
