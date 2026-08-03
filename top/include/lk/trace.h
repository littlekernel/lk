/*
 * Copyright (c) 2008-2013 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <stdio.h>

/* Trace routines. Like dprintf these are kernel diagnostics, so they go to the
 * console rather than the calling thread's stdout (see console_printf()).
 */
#define TRACE_ENTRY console_printf("%s: entry\n", __PRETTY_FUNCTION__)
#define TRACE_EXIT console_printf("%s: exit\n", __PRETTY_FUNCTION__)
#define TRACE_ENTRY_OBJ console_printf("%s: entry obj %p\n", __PRETTY_FUNCTION__, this)
#define TRACE_EXIT_OBJ console_printf("%s: exit obj %p\n", __PRETTY_FUNCTION__, this)
#define TRACE console_printf("%s:%d\n", __PRETTY_FUNCTION__, __LINE__)
#define TRACEF(str, x...) do { console_printf("%s:%d: " str, __PRETTY_FUNCTION__, __LINE__, ## x); } while (0)

/* trace routines that work if LOCAL_TRACE is set */
#define LTRACE_ENTRY do { if (LOCAL_TRACE) { TRACE_ENTRY; } } while (0)
#define LTRACE_EXIT do { if (LOCAL_TRACE) { TRACE_EXIT; } } while (0)
#define LTRACE do { if (LOCAL_TRACE) { TRACE; } } while (0)
#define LTRACEF(x...) do { if (LOCAL_TRACE) { TRACEF(x); } } while (0)
#define LTRACEF_LEVEL(level, x...) do { if (LOCAL_TRACE >= (level)) { TRACEF(x); } } while (0)
