//! Rust wrapped cbuf implementation.
//!
//! LK provides a 'cbuf' type which implements a circular buffer, using
//! spinlocks. As such, a simple wrapper around this will itself be Send/Sync.
//! And, since it is just a pointer, it is Copy/Clone.

use core::ffi::c_char;

/// Wrapper around cbufs.  These are just holding the raw pointer.
#[derive(Clone, Copy)]
pub struct Cbuf(*mut sys::cbuf);

// SAFETY: The cbuf implementation provides locking for relevant operations.
unsafe impl Sync for Cbuf {}
unsafe impl Send for Cbuf {}

impl Cbuf {
    /// Create a new cbuf from a raw pointer.
    ///
    /// # Safety
    ///
    /// The pointer must be valid and point to a properly initialized cbuf.
    pub const unsafe fn new(ptr: *mut sys::cbuf) -> Self {
        Cbuf(ptr)
    }

    /// Create and initialize a cbuf from an underlying structure.
    ///
    /// The underlying `cbuf_initialize` will malloc the buffer.
    ///
    /// # Safety
    ///
    /// This initialization is not thread safe. It will generally be done during
    /// an init function within LK, which are serialized.
    pub unsafe fn new_init(ptr: *mut sys::cbuf, size: usize) -> Self {
        let cbuf = Cbuf(ptr);
        unsafe { sys::cbuf_initialize(cbuf.0, size) };
        cbuf
    }

    // TODO: Init with provided buffer.

    /// Return the underlying raw pointer.
    pub const fn as_ptr(&self) -> *mut sys::cbuf {
        self.0
    }

    /// Return true if the cbuf is empty.
    pub fn is_empty(&self) -> bool {
        unsafe { sys::cbuf_space_avail(self.0) != 0 }
    }

    /// Return true if the cbuf is full.
    pub fn is_full(&self) -> bool {
        unsafe { sys::cbuf_space_avail(self.0) == 0 }
    }

    /// Return the number of bytes currently in the cbuf.
    ///
    /// Note that this is racy, and there is no guarantee that another thread
    /// won't change this value by reading or writing to the cbuf.
    pub fn size(&self) -> usize {
        unsafe { sys::cbuf_space_used(self.0) as usize }
    }

    /// Return the total capacity of the cbuf.
    pub fn capacity(&self) -> usize {
        // This is an inline, implement here.
        // SAFETY: These values shouldn't change after initialization.
        unsafe {
            let c = &*self.0;
            1 << c.len_pow2
        }
    }

    /// Add data to the cbuf, returning the number of bytes actually added.
    ///
    /// If `canreschedule` is true, the operation may yield to other threads.
    ///
    /// Retruns the number of bytes written.
    pub fn write(&self, data: &[u8], canreschedule: bool) -> usize {
        unsafe {
            sys::cbuf_write(self.0, data.as_ptr() as *const _, data.len(), canreschedule) as usize
        }
    }

    /// Read data from the cbuf, returning the number of bytes actually read.
    pub fn read(&self, buf: &mut [u8], block: bool) -> usize {
        unsafe { sys::cbuf_read(self.0, buf.as_mut_ptr() as *mut _, buf.len(), block) as usize }
    }

    // TODO: Peek. Safety is unclear.

    /// Write a single character to the cbuf.
    pub fn write_char(&self, c: u8, canreschedule: bool) -> bool {
        unsafe { sys::cbuf_write_char(self.0, c as c_char, canreschedule) > 0 }
    }

    /// Read a single character from the cbuf.
    pub fn read_char(&self, block: bool) -> Option<u8> {
        let mut c: c_char = 0;
        let n = unsafe { sys::cbuf_read_char(self.0, &raw mut c, block) };
        if n == 0 { None } else { Some(c as u8) }
    }
}
