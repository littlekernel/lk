// Sample application.

#![no_std]

extern crate alloc;

use alloc::boxed::Box;

use lk::{app::LkApp, lkapp};

pub fn must_link() {}

struct MyApp {
    count: usize,
}

impl LkApp for MyApp {
    fn new() -> Option<Self> {
        Some(MyApp { count: 0 })
    }

    fn main(&mut self) {
        self.count += 1;
        log::info!("App has been invoked {} times", self.count);
    }
}

lkapp!(
    c"rust_hello",
    APP_RUST_HELLO,
    pub static MY_APP: MyApp);
