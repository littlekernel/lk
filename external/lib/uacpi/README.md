# uACPI Integration for Little Kernel (LK)

This directory contains the integration of the uACPI library into the Little Kernel (LK) operating system.

## Current Version Info

- **Mainline Repository**: [https://github.com/uACPI/uACPI](https://github.com/uACPI/uACPI)
- **Commit Hash**: `d1a3de6be156edcd603ea76f6bf2f5598c48c8e7`

---

## What Was Done

1. **Source Mirroring**:
   - Placed the core uACPI source and headers from `source/` and `include/uacpi/` into `external/lib/uacpi/`.

2. **LK Integration**:
   - Created `rules.mk` to integrate the uACPI library module into the LK build system.
   - Configured compiler flags, include paths, and dependency rules.
   - Added compilation flags like `-Wno-discarded-qualifiers` to ignore compiler warnings during strict build validations.

3. **OS Services Layer (OSL) Glue**:
   - Implemented wrappers and OS dependencies in `uacpi_lk.c`. Primitives like mutexes, locks, timing, thread IDs, and memory mapping are routed through LK's kernel APIs (`mutex_acquire_timeout`, `sem_timedwait`, `spin_lock_irqsave`, `current_time_hires`, `paddr_to_kvaddr`, etc.).
   - Implemented a test shell command `uacpi` to initialize and verify the library linkage at runtime.

---

## Directions for Future Upgrades

To update the uACPI source from the upstream repository, follow these steps:

1. **Pull Newer Mainline**:
   - Check out or pull the latest changes from the official git repository: `https://github.com/uACPI/uACPI`

2. **Copy the Source**:
   - Delete/overwrite the contents of the `external/lib/uacpi/source/` directory with the new `source/` directory from upstream.
   - Delete/overwrite the contents of the `external/lib/uacpi/include/uacpi/` directory with the new `include/uacpi/` directory from upstream.

3. **Update `rules.mk`**:
   - Inspect upstream for any new C source files added under `source/`.
   - Update `MODULE_SRCS` in `external/lib/uacpi/rules.mk` if wildcard parsing is not preferred or if exclusions are needed.

4. **Update OSL Implementations (`uacpi_lk.c`)**:
   - Build the project using:
     ```bash
     make PROJECT=pc-x86-64-test
     ```
   - Resolve any undefined symbols or API signature changes introduced in newer uACPI versions by modifying `external/lib/uacpi/uacpi_lk.c`.
