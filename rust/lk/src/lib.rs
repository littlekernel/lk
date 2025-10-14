//! Rust interfaces to LK

#![no_std]

extern crate alloc;

pub extern crate lk_sys as sys;

use alloc::format;
use core::ptr::addr_of_mut;
use core::{
    alloc::{GlobalAlloc, Layout},
    ffi::{CStr, c_uint, c_void},
    panic::PanicInfo,
};

pub mod cbuf;
pub mod init;
pub mod lkonce;
pub mod log;
pub mod macros;

/// Initialize the rust side of the code.
///
/// Due to linker optimization, without a direct call into this code, the linker will not bring in
/// anything.
pub fn init() {
    unsafe {
        sys::fputs(c"*** Rust init ***\n".as_ptr(), lk_stderr());
    }
}

/// Return the current stderr from lk's simple stdio implementation.
#[inline(always)]
fn get_stdfile(index: usize) -> *mut sys::FILE {
    // SAFETY: `__stdio_FILEs` is  defined as an `[]` array in C, which doesn\t have a rust
    // equivalent.  Bindgen generates a zero-element array.  However, Rust does require that static
    // symbols be at an afddress (even if zero sizesd) so this symbol will correspond to the first
    // element of the C array.  We cast this to the first element so that we can then do pointer
    // arithmetic to get the subsequent elements.  stderr is '2' per long history, as well as being
    // defined that way in POSIX.
    unsafe {
        let stdio = addr_of_mut!(sys::__stdio_FILEs) as *mut sys::FILE;
        stdio.add(index)
    }
}

/// Return the current stderr from lk's simple stdio implementation.
#[inline(always)]
pub fn lk_stderr() -> *mut sys::FILE {
    get_stdfile(2)
}

/// Return the current stdout from lk's simple stdio implementation.
#[inline(always)]
pub fn lk_stdout() -> *mut sys::FILE {
    get_stdfile(1)
}

/// Return the current stdin from lk's simple stdio implementation.
#[inline(always)]
pub fn lk_stdin() -> *mut sys::FILE {
    get_stdfile(0)
}

extern "C" fn sample_rust_hook(_level: c_uint) {
    unsafe {
        sys::fputs(c"*** INIT: lk init\n".as_ptr(), lk_stderr());
    }
    ::log::warn!("Warning: Message here");
}

/*
#[unsafe(link_section = "lk_init")]
#[used]
static SETUP_RUST_HOOK: sys::lk_init_struct = sys::lk_init_struct {
    level: init::lk_init_level::LK_INIT_LEVEL_PLATFORM.0 as u16,
    flags: init::lk_init_flags::LK_INIT_FLAG_PRIMARY_CPU.0 as u16,
    hook: Some(sample_rust_hook),
    name: c"lk_rust_hook".as_ptr(),
};
*/
LK_INIT_HOOK!(
    SETUP_RUST_HOOK,
    sample_rust_hook,
    init::lk_init_level::LK_INIT_LEVEL_PLATFORM
);

pub struct LkAllocator;

unsafe impl GlobalAlloc for LkAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        let size = layout.size();

        // TODO: What is the correct size?  Bindgen put u64 instead of size_t
        unsafe { sys::malloc(size) as *mut u8 }
    }

    unsafe fn dealloc(&self, ptr: *mut u8, _layout: Layout) {
        unsafe { sys::free(ptr as *mut c_void) }
    }
}

#[global_allocator]
pub static LK_ALLOCATOR: LkAllocator = LkAllocator;

#[panic_handler]
fn handle_panic(info: &PanicInfo) -> ! {
    let panic_message = format!("{info}\0");
    let panic_message_c = CStr::from_bytes_with_nul(panic_message.as_bytes())
        .expect("Unexpected null byte in panic message");
    unsafe { sys::panic(c"Rust in LK %s\n".as_ptr(), panic_message_c.as_ptr()) }
}
