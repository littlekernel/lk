//! Little Kernel style init for statics
//!
//! LK commonly uses an initialization pattern where a value is declared
//! statically (usually zero initialized) and then an initialization function is
//! either called during early init, or a specific init function is registered.
//!
//! In order to handle this from Rust, we provide this LkOnce type, which wraps
//! a value that is intended to be initialized in this manner.  In addition to
//! the initialization happening from a function, we also support initialization
//! in-place, which is needed for many things in LK, as the LK list type uses
//! self-referential pointers.
//!
//! Once initialized, the value is only accessible with immutable pointers.

use core::{
    cell::UnsafeCell,
    mem::MaybeUninit,
    sync::atomic::{AtomicU8, Ordering},
};

pub struct LkOnce<T> {
    value: UnsafeCell<MaybeUninit<T>>,
    state: AtomicU8,
}

// SAFETY: The purpose of this code is to require the item be initialized
// safely, and then to ensure it is only used in a read-only manner afterwards.
// We are only Sync, however, if the inner value is also Sync.
unsafe impl<T: Sync> Sync for LkOnce<T> {}

/// States for the initialization process.
///
/// Diagram is as follows:
///
///     Uninit -> EarlyInitting -> EarlyInitted -> Initializing -> Initialized
///                      \______________________________/
///
/// Early init can optionally be called before init, but must be called before.
/// Note that trying to initialize out of sequence can briefly leave the atomic
/// in one of the initting states before panicing.  Worst cause will cause the
/// use on another CPU to panic first instead of the clearer one in the init
/// function.
enum States {
    Uninit = 0,
    EarlyInitting = 1,
    EarlyInitted = 2,
    Initializing = 3,
    Initialized = 4,
}

impl<T> LkOnce<T> {
    pub const fn uninit() -> Self {
        Self {
            value: UnsafeCell::new(MaybeUninit::uninit()),
            state: AtomicU8::new(States::Uninit as u8),
        }
    }

    /// Perform an optional early initialization.
    ///
    /// This must be called before `init` is called, and only called once.
    ///
    /// # Safety
    ///
    /// Although this is giving a raw pointer to the given function, that
    /// function will need to be unsafe to use it.  The value is not considered
    /// initialized until the `init` function is called.
    pub fn early_init(&self, init: impl FnOnce(*mut T)) {
        let prev_state = self
            .state
            .swap(States::EarlyInitting as u8, Ordering::SeqCst);

        // It is only valid to initialize early from fully uninitialized.
        if prev_state == States::Uninit as u8 {
            let value = unsafe { &mut *self.value.get() };
            init(value.as_mut_ptr());
            self.state
                .store(States::EarlyInitted as u8, Ordering::Release);
        } else {
            panic!("LkOnce early_init called out of order");
        }
    }

    /// Perform the real initialization of the item.
    ///
    /// This must be called exactly once, after an optional call to
    /// `early_init`. After this returns, the value is considered fully
    /// initialized and can be used immutably.
    ///
    /// # Safety
    ///
    /// This function is not safe because we have no guarantee that the init
    /// function initializes the value correctly.
    pub unsafe fn init(&self, init: impl FnOnce(*mut T)) {
        let prev_state = self
            .state
            .swap(States::Initializing as u8, Ordering::SeqCst);

        // It is valid to init from either entirely uninitialized, or after
        // the early init has completed.
        if prev_state == States::Uninit as u8 || prev_state == States::EarlyInitted as u8 {
            let value = unsafe { &mut *self.value.get() };
            init(value.as_mut_ptr());
            self.state
                .store(States::Initialized as u8, Ordering::Release);
        } else {
            panic!("LkOnce init called out of order");
        }
    }

    /// Get the value, ensuring that it has been properly initialized.
    pub fn get(&self) -> &T {
        let state = self.state.load(Ordering::Acquire);
        if state == States::Initialized as u8 {
            let value = unsafe { &*self.value.get() };
            unsafe { &*value.as_ptr() }
        } else {
            panic!("LkOnce get called before initialized");
        }
    }

    /// Sometimes it it useful to get the device and only require that early init have been called.
    pub fn get_early(&self) -> &T {
        let state = self.state.load(Ordering::Acquire);
        if state == States::Initialized as u8 || state == States::EarlyInitted as u8 {
            let value = unsafe { &*self.value.get() };
            unsafe { &*value.as_ptr() }
        } else {
            panic!("LkOnce get_early called before early initialized");
        }
    }
}
