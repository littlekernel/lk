/*
 * Copyright (c) 2008-2012 Travis Geiselbrecht
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

#include <stdarg.h>
#include <stddef.h>
#include <compiler.h>
#include <platform/debug.h>
#include <list.h>

#if !defined(LK_DEBUGLEVEL)
#define LK_DEBUGLEVEL 0
#endif

/* debug levels */
#define CRITICAL 0
#define ALWAYS 0
#define INFO 1
#define SPEW 2

typedef struct __print_callback print_callback_t;
struct __print_callback {
	struct list_node entry;
	void (*print)(print_callback_t *cb, const char *str, size_t len);
};

__BEGIN_CDECLS

#if !DISABLE_DEBUG_OUTPUT

/* input/output */
void _dputc(char c);
int _dputs(const char *str);
int _dwrite(const char *ptr, size_t len);
int _dprintf(const char *fmt, ...) __PRINTFLIKE(1, 2);
int _dvprintf(const char *fmt, va_list ap);

/* dump memory */
void hexdump(const void *ptr, size_t len);
void hexdump8(const void *ptr, size_t len);

#else

/* input/output */
static inline void _dputc(char c) { }
static inline int _dputs(const char *str) { return 0; }
static inline int _dwrite(const char *ptr, size_t len) { return 0; }
static inline int __PRINTFLIKE(1, 2) _dprintf(const char *fmt, ...) { return 0; }
static inline int _dvprintf(const char *fmt, va_list ap) { return 0; }

/* dump memory */
static inline void hexdump(const void *ptr, size_t len) { }
static inline void hexdump8(const void *ptr, size_t len) { }

#endif /* DISABLE_DEBUG_OUTPUT */

/* register callback to receive debug prints */
void register_print_callback(print_callback_t *cb);
void unregister_print_callback(print_callback_t *cb);

#define dputc(level, str) do { if ((level) <= LK_DEBUGLEVEL) { _dputc(str); } } while (0)
#define dputs(level, str) do { if ((level) <= LK_DEBUGLEVEL) { _dputs(str); } } while (0)
#define dwrite(level, ptr, len) do { if ((level) <= LK_DEBUGLEVEL) { _dwrite(ptr, len); } } while(0)
#define dprintf(level, x...) do { if ((level) <= LK_DEBUGLEVEL) { _dprintf(x); } } while (0)
#define dvprintf(level, x...) do { if ((level) <= LK_DEBUGLEVEL) { _dvprintf(x); } } while (0)

/* systemwide halts */
void _panic(void *caller, const char *fmt, ...) __PRINTFLIKE(2, 3) __NO_RETURN;
#define panic(x...) _panic(__GET_CALLER(), x)

#define PANIC_UNIMPLEMENTED panic("%s unimplemented\n", __PRETTY_FUNCTION__)

/* spin the cpu for a period of (short) time */
void spin(uint32_t usecs);

/* spin the cpu for a certain number of cpu cycles */
void spin_cycles(uint32_t usecs);

__END_CDECLS

#endif
