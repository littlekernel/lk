# LK coding style and conventions

- Formatting is controlled by `.clang-format`.
- 4-space indentation, no tabs, no trailing whitespace.
- K&R braces (`if (x) { ... }`).
- Pointer style uses right-binding (`void *ptr`).
- Header guards should use `#pragma once`.
- Keep lines near <=100 columns when practical.

## Build/module conventions
- Each module defines `LOCAL_DIR := $(GET_LOCAL_DIR)` and `MODULE := $(LOCAL_DIR)`.
- Use `$(LOCAL_DIR)/...` for module source paths.
- Declare dependencies via `MODULE_DEPS`.
- Finish module files with `include make/module.mk`.

## C/C++ constraints
- Freestanding environment assumptions (`-ffreestanding`).
- C++ is restricted (`--std=c++14`, no exceptions, no RTTI, no thread-safe statics).
- Prefer existing LK patterns/macros for apps and static console command registration.
