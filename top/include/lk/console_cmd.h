/*
 * Copyright (c) 2008-2009 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

__BEGIN_CDECLS

/* command args */
typedef struct {
    const char *str;
    unsigned long u;
    unsigned long long ull;
    void *p;
    long i;
    bool b;
} console_cmd_args;

typedef int (*console_cmd_func)(int argc, const console_cmd_args *argv);

#define CMD_AVAIL_NORMAL (0x1 << 0)
#define CMD_AVAIL_PANIC  (0x1 << 1)
#define CMD_AVAIL_ALWAYS (CMD_AVAIL_NORMAL | CMD_AVAIL_PANIC)

__END_CDECLS

/* Register a static block of commands at init time when lib/console is
 * paret of the build. Otherwise stub out these definitions so that they do
 * not get included.
 */
#if WITH_LIB_CONSOLE

/* pull the equivalent strctures and macros out of lib/console's headers */
#include <lib/console/cmd.h>

#else

/* no command blocks, so null them out */
#define STATIC_COMMAND_START
#define STATIC_COMMAND_END(name)
#define STATIC_COMMAND_START_NAMED(name)
#define STATIC_COMMAND_END_NAMED(name)

#define STATIC_COMMAND(command_str, help_str, func)
#define STATIC_COMMAND_MASKED(command_str, help_str, func, availability_mask)


#endif
