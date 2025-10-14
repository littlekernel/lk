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

// TODO: replace with `trusty-log` crate once it is `no_std`-compatible

use alloc::ffi::CString;
use alloc::format;
use core::ffi::c_uint;
use log::{LevelFilter, Log, Metadata, Record};

use crate::LK_INIT_HOOK;
use crate::init::lk_init_level;

use crate::lk_stderr;
use crate::sys::fflush;
use crate::sys::fputs;
// use crate::sys::LK_LOGLEVEL_RUST;
// TODO: Configure this properly.
static LK_LOGLEVEL_RUST: usize = 4;

static TRUSTY_LOGGER: TrustyKernelLogger = TrustyKernelLogger;

pub struct TrustyKernelLogger;

impl Log for TrustyKernelLogger {
    fn enabled(&self, _metadata: &Metadata) -> bool {
        true
    }

    fn log(&self, record: &Record) {
        // unsafe { fputs(c"Log a message\n".as_ptr(), lk_stderr()); }
        if self.enabled(record.metadata()) {
            let cstr = CString::new(format!("{} - {}\n", record.level(), record.args())).unwrap();
            // Safety:
            // The pointer returned by `cstr.as_ptr()` is valid because the lifetime of the
            // `CString` encompasses the lifetime of the unsafe block.
            // `lk_stderr()` returns a FILE pointer that is valid or null.
            unsafe { fputs(cstr.as_ptr(), lk_stderr()) };
        }
    }

    fn flush(&self) {
        // Safety:
        // `lk_stderr()` returns a FILE pointer that is valid or null.
        unsafe { fflush(lk_stderr()) };
    }
}

/// Initialize logging for Rust in the kernel
///
/// By default, only warnings and errors are logged (even in debug builds).
///
/// The log level (`LK_LOGLEVEL_RUST`) is controlled by these make variables:
/// - `LOG_LEVEL_KERNEL_RUST` if set,
/// - `LOG_LEVEL_KERNEL` if set, and
/// - `DEBUG` otherwise.
///
/// Values below (above) expected values sets the log level to off (trace).
extern "C" fn kernel_log_init_func(_level: c_uint) {
    unsafe {
        fputs(c"*** Setup logging\n".as_ptr(), lk_stderr());
    }
    log::set_logger(&TRUSTY_LOGGER).unwrap();
    // Level or LevelFilter cannot be created directly from integers
    // https://github.com/rust-lang/log/issues/460
    //
    // bindgen emits `LK_LOGLEVEL_RUST` as `u32` when the value is
    // a positive integer and omits it otherwise thus causing the
    // build to fail.
    log::set_max_level(match LK_LOGLEVEL_RUST {
        0 => LevelFilter::Off,
        1 => LevelFilter::Error,
        2 => LevelFilter::Warn, // the default for Trusty
        3 => LevelFilter::Info,
        4 => LevelFilter::Debug,
        _ => LevelFilter::Trace, // enable trace! at 5+
    });
}

// LK_INIT_HOOK!(KERNEL_LOG_INIT, kernel_log_init_func, lk_init_level::LK_INIT_LEVEL_HEAP);
LK_INIT_HOOK!(
    KERNEL_LOG_INIT,
    kernel_log_init_func,
    lk_init_level::LK_INIT_LEVEL_ARCH
);
