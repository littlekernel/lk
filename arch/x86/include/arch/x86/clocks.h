/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>
#include <stdint.h>

__BEGIN_CDECLS

// What the cpu itself can say about the clocks it is fed by, without measuring anything.
// Every function returns 0 when the answer isn't knowable exactly, in which case the
// caller has to calibrate against another clock. None of these are trusted blindly by
// the platform timer code either: whatever they claim is cross checked against the HPET
// when there is one.

// The core crystal clock the TSC and local apic timer are derived from. From cpuid 0x15
// ecx when the part enumerates it, else from the microarch table on bare metal.
uint64_t x86_cpu_crystal_hz(void);

// The TSC's nominal frequency. Intel: crystal * cpuid 0x15 ebx/eax. AMD Zen: decoded from
// the P0 P-state MSR, on bare metal only. Never nonzero on a part whose TSC follows the
// core clock.
uint64_t x86_cpu_tsc_hz(void);

// The local apic timer's input clock. Only claimed on bare metal, since an emulated apic
// timer runs at whatever rate the hypervisor picked regardless of what the cpuid it
// passes through says about the host's crystal.
uint64_t x86_cpu_lapic_timer_hz(void);

// The processor base frequency from cpuid 0x16, in MHz. This is a marketing number rounded
// to 100MHz (an i7-12700 says 2100 while its TSC ticks at 2112MHz), so it's good enough for
// a plausibility check but never used as a frequency source.
uint32_t x86_cpu_base_mhz(void);

// Human readable description of where x86_cpu_tsc_hz() got its answer, for boot output.
const char *x86_cpu_tsc_freq_source_name(void);

__END_CDECLS
