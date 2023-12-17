/*
 * Copyright (c) 2012 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <arch/arm/cm.h>
#include <kernel/thread.h>
#include <lk/compiler.h>
#include <stdint.h>

/*
 * Make a nice 8 byte aligned stack to run on before the threading system is up.
 * Put it in the .bss.prebss.* section to make sure it doesn't get wiped
 * when bss is cleared a little ways into boot.
 */
static uint8_t initial_stack[1024] __SECTION(".bss.prebss.initial_stack") __ALIGNED(8);

extern void _start(void);
extern void _nmi(void);
extern void _hardfault(void);
extern void _memmanage(void);
extern void _busfault(void);
extern void _usagefault(void);
extern void _svc(void);
extern void _debugmonitor(void);
extern void _pendsv(void);
extern void _systick(void);

#if defined(WITH_DEBUGGER_INFO)
// Contains sufficient information for a remote debugger to walk
// the thread list without needing the symbols and debug sections in
// the elf binary for lk or the ability to parse them.
static const struct __debugger_info__ {
    u32 version; // flags:16 major:8 minor:8
    void *thread_list_ptr;
    void *current_thread_ptr;
    u8 off_list_node;
    u8 off_state;
    u8 off_saved_sp;
    u8 off_name;
    u8 off_waitq;
} _debugger_info = {
    .version = 0x0200,
    .thread_list_ptr = &thread_list,
    .current_thread_ptr = &_current_thread,
    .off_list_node = __builtin_offsetof(thread_t, thread_list_node),
    .off_state = __builtin_offsetof(thread_t, state),
    .off_saved_sp = __builtin_offsetof(thread_t, arch.sp),
    .off_name = __builtin_offsetof(thread_t, name),
    .off_waitq = __builtin_offsetof(thread_t, blocking_wait_queue),
};
#endif

// ARMv7m+ have more vectors than armv6m
#if (__CORTEX_M >= 0x03) || (CORTEX_SC >= 300)
#define ARMV7M_VECTOR(v) (v)
#else
#define ARMV7M_VECTOR(v) 0
#endif

const void *const __SECTION(".text.boot.vectab1") vectab[] = {
    /* arm exceptions */
    initial_stack + sizeof(initial_stack),
    _start,
    _nmi, // nmi
    _hardfault, // hard fault
    ARMV7M_VECTOR(_memmanage), // mem manage
    ARMV7M_VECTOR(_busfault), // bus fault
    ARMV7M_VECTOR(_usagefault), // usage fault
    0, // reserved
#if defined(WITH_DEBUGGER_INFO)
    // stick a pointer to the debugger info structure in unused vectors
    (void *) 0x52474244,
    &_debugger_info,
#else
    0, // reserved
    0, // reserved
#endif
    0, // reserved
    _svc, // svcall
    ARMV7M_VECTOR(_debugmonitor), // debug monitor
    0, // reserved
    _pendsv, // pendsv
    _systick, // systick
};

