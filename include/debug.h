/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef __DEBUG_H
#define __DEBUG_H

#include <assert.h>
#include <stdarg.h>
#include <compiler.h>
#include <platform/debug.h>
#include <printf.h>

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(DEBUG)
#define DEBUGLEVEL DEBUG
#else
#define DEBUGLEVEL 2
#endif

/* debug levels */
#define CRITICAL 0
#define ALWAYS 0
#define INFO 1
#define SPEW 2

/* output */
void _dputc(char c); // XXX for now, platform implements
int _dputs(const char *str);
int _dprintf(const char *fmt, ...) __PRINTFLIKE(1, 2);
int _dvprintf(const char *fmt, va_list ap);

#define dputc(level, str) do { if ((level) <= DEBUGLEVEL) { _dputc(str); } } while (0)
#define dputs(level, str) do { if ((level) <= DEBUGLEVEL) { _dputs(str); } } while (0)
#define dprintf(level, x...) do { if ((level) <= DEBUGLEVEL) { _dprintf(x); } } while (0)
#define dvprintf(level, x...) do { if ((level) <= DEBUGLEVEL) { _dvprintf(x); } } while (0)

/* input */
int dgetc(char *c, bool wait);

/* systemwide halts */
void halt(void) __NO_RETURN;

void _panic(void *caller, const char *fmt, ...) __PRINTFLIKE(2, 3) __NO_RETURN;
#define panic(x...) _panic(__GET_CALLER(), x)

#define PANIC_UNIMPLEMENTED panic("%s unimplemented\n", __PRETTY_FUNCTION__)

/* spin the cpu for a period of (short) time */
void spin(uint32_t usecs);

/* dump memory */
void hexdump(const void *ptr, size_t len);
void hexdump8(const void *ptr, size_t len);

/* trace routines */
#define TRACE_ENTRY printf("%s: entry\n", __PRETTY_FUNCTION__)
#define TRACE_EXIT printf("%s: exit\n", __PRETTY_FUNCTION__)
#define TRACE_ENTRY_OBJ printf("%s: entry obj %p\n", __PRETTY_FUNCTION__, this)
#define TRACE_EXIT_OBJ printf("%s: exit obj %p\n", __PRETTY_FUNCTION__, this)
#define TRACE printf("%s:%d\n", __PRETTY_FUNCTION__, __LINE__)
#define TRACEF(x...) do { printf("%s:%d: ", __PRETTY_FUNCTION__, __LINE__); printf(x); } while (0)

/* trace routines that work if LOCAL_TRACE is set */
#define LTRACE_ENTRY do { if (LOCAL_TRACE) { TRACE_ENTRY; } } while (0)
#define LTRACE_EXIT do { if (LOCAL_TRACE) { TRACE_EXIT; } } while (0)
#define LTRACE do { if (LOCAL_TRACE) { TRACE; } } while (0)
#define LTRACEF(x...) do { if (LOCAL_TRACE) { TRACEF(x); } } while (0)

#if defined(__cplusplus)
}
#endif

#endif
