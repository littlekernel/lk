/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>
#include <sys/types.h>

__BEGIN_CDECLS

// Possible boot args passed from various arch's start.S
extern ulong lk_boot_args[4];

// Main entry point to the OS on the boot cpu. Called from low level arch code after getting the
// boot cpu into enough of a known state to enter C code.
void lk_main(ulong arg0, ulong arg1, ulong arg2, ulong arg3) __NO_RETURN __EXTERNALLY_VISIBLE;

#if WITH_SMP
// Before starting any secondary cpus, the boot cpu should call this function to set up
// the secondary cpu idle and bootstrap threads.
void lk_init_secondary_cpus(uint secondary_cpu_count);

// High level entry point for secondary cpus. Called from low level arch code to bootstrap the
// threading system on the secondary cpu and call any secondary cpu init routines up to
// LK_INIT_LEVEL_THREADING.
void lk_secondary_cpu_entry_early(void);

// Final entry point for secondary cpus from arch code, running the rest of the secondary cpu
// init routines and entering the scheduler. Does not return.
void lk_secondary_cpu_entry(void) __NO_RETURN;
#endif

__END_CDECLS
