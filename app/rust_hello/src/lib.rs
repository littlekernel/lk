// Sample application.

#![no_std]

use core::ffi::c_void;

use lk::sys::{APP_FLAG_NO_AUTOSTART, app_descriptor};

pub fn must_link() {}

/// Manual, and unsafe declration of an app.
#[unsafe(link_section = "apps")]
#[used]
static APP_RUST_HELLO: app_descriptor = app_descriptor {
    name: c"rust_hello".as_ptr(),
    init: Some(init),
    entry: Some(main),
    flags: APP_FLAG_NO_AUTOSTART,
    stack_size: 0,
};

extern "C" fn init(_desc: *const app_descriptor) {
    log::info!("Rust hello app init");
}

extern "C" fn main(_desc: *const app_descriptor, _args: *mut c_void) {
    log::info!("Rust hello app main");
}
