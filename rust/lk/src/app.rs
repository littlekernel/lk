//! Application support
//!
//! Applications in LK are managed through a descriptor, the combines a name,
//! some flags, a possible stack size override, with an `init` and `main`
//! function pointer.
//!
//! This Rust module provides a more Rust-friendly way to define applications.
//! Applications for Rust consist of someting that implements the `LkApp` trait.
//! The `init` function from lk is used to invoke the constructor for this
//! object. Then each invocation of `main` will invoke the `main` method on the
//! object. Because of multiple, uncoordinated invocations of `main`, a given
//! call from lk on `main` will only invoke the `main` method after previous
//! invocations have completed. This allows the struct to only require `Send`
//! (as it is sent to the thread created for the app), and not `Sync`.

/// Something that implements an LK application.  The 'new' method is invoked
/// during initialization time to create one of these, and then the 'main'
/// method is invoked each time the application is started.
pub trait LkApp: Sized + Send + 'static {
    /// Construct a new instance of the application.
    ///
    /// If "None" is returned, the application will not be started.
    fn new() -> Option<Self>;

    /// Application main function.
    fn main(&mut self);
}

#[macro_export]
macro_rules! lkapp {
    ($cname:literal,
     $appname:ident,
     $vis:vis static $name:ident: $app:ty
    ) => {
        $vis static $name: ::core::sync::atomic::AtomicPtr<$app> = ::core::sync::atomic::AtomicPtr::new(::core::ptr::null_mut());

        #[unsafe(link_section = "apps")]
        #[used]
        static $appname: $crate::sys::app_descriptor = $crate::sys::app_descriptor {
            name: $cname.as_ptr(),
            init: Some(init),
            entry: Some(main),
            flags: $crate::sys::APP_FLAG_NO_AUTOSTART,
            stack_size: 0,
        };

        extern "C" fn init(_desc: *const $crate::sys::app_descriptor) {
            ::log::info!("Rust app init");

            let app = match <$app>::new() {
                Some(a) => a,
                None => {
                    ::log::error!("App constructor returned None");
                    return;
                }
            };

            let app_box = Box::new(app);
            let app_ptr = Box::into_raw(app_box);
            $name.store(app_ptr, ::core::sync::atomic::Ordering::SeqCst);
        }

        extern "C" fn main(_desc: *const $crate::sys::app_descriptor, _args: *mut ::core::ffi::c_void) {
            let app_ptr = $name.load(::core::sync::atomic::Ordering::SeqCst);
            if !app_ptr.is_null() {
                // Safe because we ensure only one 'main' runs at a time.
                let app = unsafe { &mut *app_ptr };
                app.main();
                return;
            }
            ::log::error!("App already running");
        }
    };
}
