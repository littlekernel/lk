/*
 * Copyright (c) 2008-2013 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <stdio.h>
#include <kernel/debug.h>

/* trace routines */
#define TRACE_ENTRY printf("%s: entry\n", __PRETTY_FUNCTION__)
#define TRACE_EXIT printf("%s: exit\n", __PRETTY_FUNCTION__)
#define TRACE_ENTRY_OBJ printf("%s: entry obj %p\n", __PRETTY_FUNCTION__, this)
#define TRACE_EXIT_OBJ printf("%s: exit obj %p\n", __PRETTY_FUNCTION__, this)
#define TRACE printf("%s:%d\n", __PRETTY_FUNCTION__, __LINE__)
#define TRACEF(str, x...) do { printf("%s:%d: " str, __PRETTY_FUNCTION__, __LINE__, ## x); } while (0)

/* trace routines that work if LOCAL_TRACE is set */
#define LTRACE_ENTRY do { if (LOCAL_TRACE) { TRACE_ENTRY; } } while (0)
#define LTRACE_EXIT do { if (LOCAL_TRACE) { TRACE_EXIT; } } while (0)
#define LTRACE do { if (LOCAL_TRACE) { TRACE; } } while (0)
#define LTRACEF(x...) do { if (LOCAL_TRACE) { TRACEF(x); } } while (0)
#define LTRACEF_LEVEL(level, x...) do { if (LOCAL_TRACE >= (level)) { TRACEF(x); } } while (0)

/* kprintf versions of the above */
#define KTRACE_ENTRY kprintf("%s: entry\n", __PRETTY_FUNCTION__)
#define KTRACE_EXIT kprintf("%s: exit\n", __PRETTY_FUNCTION__)
#define KTRACE_ENTRY_OBJ kprintf("%s: entry obj %p\n", __PRETTY_FUNCTION__, this)
#define KTRACE_EXIT_OBJ kprintf("%s: exit obj %p\n", __PRETTY_FUNCTION__, this)
#define KTRACE kprintf("%s:%d\n", __PRETTY_FUNCTION__, __LINE__)
#define KTRACEF(str, x...) do { kprintf("%s:%d: " str, __PRETTY_FUNCTION__, __LINE__, ## x); } while (0)

#define KLTRACE_ENTRY do { if (LOCAL_TRACE) { KTRACE_ENTRY; } } while (0)
#define KLTRACE_EXIT do { if (LOCAL_TRACE) { KTRACE_EXIT; } } while (0)
#define KLTRACE do { if (LOCAL_TRACE) { KTRACE; } } while (0)
#define KLTRACEF(x...) do { if (LOCAL_TRACE) { KTRACEF(x); } } while (0)
#define KLTRACEF_LEVEL(level, x...) do { if (LOCAL_TRACE >= (level)) { KTRACEF(x); } } while (0)
