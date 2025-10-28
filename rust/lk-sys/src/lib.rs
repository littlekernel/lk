//! Rust support for LK
//
// Copyright (c) 2025 Linaro Ltd.
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#![no_std]
#![allow(non_camel_case_types)]
#![allow(non_upper_case_globals)]

use core::ffi::c_char;

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

unsafe impl Sync for lk_init_struct {}
unsafe impl Sync for app_descriptor {}

// lk_init_level constants have large gaps between them and some modules
// add or subtract from these constants to indicate that it wants to run
// right before or after other init hooks at a given level. Add add and sub
// functions to lk_init_level to allow this for rust init hooks as well.
impl lk_init_level {
    pub const fn add(mut self, rhs: uint) -> Self {
        self.0 += rhs;
        self
    }
    pub const fn sub(mut self, rhs: uint) -> Self {
        self.0 -= rhs;
        self
    }
}

impl lk_init_struct {
    pub const fn new(
        level: lk_init_level,
        flags: lk_init_flags,
        hook: unsafe extern "C" fn(uint),
        name: *const c_char,
    ) -> Self {
        lk_init_struct {
            level: level.0 as u16,
            flags: flags.0 as u16,
            hook: Option::Some(hook),
            name,
        }
    }
}
