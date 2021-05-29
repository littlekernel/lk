/*
 * Copyright (c) 2008-2009 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>

__BEGIN_CDECLS

/* Previously, this file was included to get access to defining a console
 * command. This logic has been moved into the following header, which is
 * what in almost every case what regular code wants to include instead of
 * this file.
 */
#include <lk/console_cmd.h>
#include <lib/console/cmd.h>

typedef struct console console_t;

/* create an instance of the console */
/* TODO: actually implement the history option. Currently always implements history according
 * to the build variable CONSOLE_ENABLE_HISTORY. */
console_t *console_create(bool with_history);

/* Run the main console loop. Will set the current console TLS pointer as a side effect */
void console_start(console_t *con);

/* Routines to let code directly run commands in an existing console */
/* NOTE: Passing null as first argument selects the current console associated with the current thread */
int console_run_script(console_t *con, const char *string);
int console_run_script_locked(console_t *con, const char *string); // special case from inside a command
void console_abort_script(console_t *con);

/* Get/set the current console in the thread's TLS slot reserved for it.
 * New threads will inherit the pointer from the parent thread.
 *
 * TODO: use a ref count to keep the console from being destroyed from underneath it.
 */
console_t *console_get_current(void);
console_t *console_set_current(console_t *con); // returns old console pointer

console_cmd_func console_get_command_handler(const char *command);

/* panic shell api */
void panic_shell_start(void);

/* enable the panic shell if we're being built */
#if !defined(ENABLE_PANIC_SHELL) && PLATFORM_SUPPORTS_PANIC_SHELL
#define ENABLE_PANIC_SHELL 1
#endif

__END_CDECLS
