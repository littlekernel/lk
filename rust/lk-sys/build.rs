//! Build support for Rust on LK
//
// Copyright (c) 2025 Linaro Ltd.
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

use std::{env, path::PathBuf};

use bindgen::Builder;

fn main() -> anyhow::Result<()> {
    // Bindgen uses it's own algorithms to find libclang.  Sometimes, it might find a different llvm
    // version than the version used to compile the code.  We're making a basic assumption that the
    // code is either being built with LLVM, or that GCC and LLVM are compatible enough for this to
    // generate correct bindings.
    let version = bindgen::clang_version();
    println!("Clang version: {:?}", version);

    // Use the llvm target passed in the environment.
    let target = env::var("TARGET")?;
    let target_arg = format!("--target={}", target);

    let out_path = PathBuf::from(env::var("OUT_DIR")?);

    let bindings = Builder::default()
        .header("wrapper.h")
        .use_core()
        .clang_arg(&target_arg);

    let bindings = convert_include_path(bindings)?;

    // Allow `SYS_INCLUDES` to be specified to specify a path where the system includes for this
    // version of clang.
    let bindings = if let Ok(sys_inc) = env::var("SYS_INCLUDE") {
        bindings.clang_args(&["-isystem", &sys_inc])
    } else {
        bindings
    };

    let bindings = bindings
        .clang_arg("-ffreestanding")
        .prepend_enum_name(false)
        .newtype_enum("lk_init_level")
        .newtype_enum("lk_init_flags")
        .allowlist_function("fflush")
        .allowlist_function("fputs")
        .allowlist_function("panic")
        .allowlist_function("malloc")
        .allowlist_function("free")
        .allowlist_type("obj_ref")
        .allowlist_item("__stdio_FILEs")
        .allowlist_type("cbuf")
        .allowlist_function("cbuf.*")
        .allowlist_type("lk_init_level")
        .allowlist_type("lk_init_flags")
        .allowlist_type("lk_init_struct")
        .allowlist_function("register_int_handler")
        .allowlist_function("unmask_interrupt")
        .allowlist_type("app_descriptor")
        .allowlist_item("APP_FLAG.*")
        .generate()?;

    bindings.write_to_file(out_path.join("bindings.rs"))?;

    Ok(())
}

/// Convert the provided include path to properly work.  There are two issues here:
/// The GLOBAL_INCLUDES variable contains paths with a leading `-I` on them, which we need to strip.
/// In addition, the paths are relative to the top of the build tree.  Cargo invokes the build.rs
/// code while within the crate directory.
///
/// For now, we'll assume we are two directories deep and build the paths appropriately.
fn convert_include_path(bindings: Builder) -> anyhow::Result<Builder> {
    let arg = env::var("GLOBAL_INCLUDES")?;
    let mut bindings = bindings;

    for entry in arg.split(&[' ']) {
        if entry.is_empty() {
            continue;
        }

        // Leave absolute paths alone.
        if entry.starts_with("-I/") {
            bindings = bindings.clang_arg(entry);
            continue;
        }

        if entry.starts_with("-I") {
            let arg = format!("-I../../{}", &entry[2..]);
            bindings = bindings.clang_arg(arg);
            continue;
        }

        // Bare abs paths, need the -I.
        if entry.starts_with("/") {
            let arg = format!("-I{}", &entry);
            bindings = bindings.clang_arg(arg);
            continue;
        }

        // Otherwise, make relative.
        let arg = format!("-I../../{}", &entry);
        bindings = bindings.clang_arg(arg);
    }

    Ok(bindings)
}
