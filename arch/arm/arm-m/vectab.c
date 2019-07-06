/*
 * Copyright (c) 2012 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
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
extern struct __debugger_info__ _debugger_info;
#endif

const void *const __SECTION(".text.boot.vectab1") vectab[] = {
    /* arm exceptions */
    initial_stack + sizeof(initial_stack),
    _start,
    _nmi, // nmi
    _hardfault, // hard fault
    _memmanage, // mem manage
    _busfault, // bus fault
    _usagefault, // usage fault
    0, // reserved
#if defined(WITH_DEBUGGER_INFO)
    (void *) 0x52474244,
    &_debugger_info,
#else
    0, // reserved
    0, // reserved
#endif
    0, // reserved
    _svc, // svcall
    _debugmonitor, // debug monitor
    0, // reserved
    _pendsv, // pendsv
    _systick, // systick
};



