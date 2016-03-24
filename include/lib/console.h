/*
 * Copyright (c) 2008-2009 Travis Geiselbrecht
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
#ifndef __LIB_CONSOLE_H
#define __LIB_CONSOLE_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>
#include <compiler.h>

/* command args */
typedef struct {
    const char *str;
    unsigned long u;
    void *p;
    long i;
    bool b;
} cmd_args;

typedef int (*console_cmd)(int argc, const cmd_args *argv);

#define CMD_AVAIL_NORMAL (0x1 << 0)
#define CMD_AVAIL_PANIC  (0x1 << 1)
#define CMD_AVAIL_ALWAYS (CMD_AVAIL_NORMAL | CMD_AVAIL_PANIC)

/* a block of commands to register */
typedef struct {
    const char *cmd_str;
    const char *help_str;
    const console_cmd cmd_callback;
    uint8_t availability_mask;
} cmd;

typedef struct _cmd_block {
    struct _cmd_block *next;
    size_t count;
    const cmd *list;
} cmd_block;

/* register a static block of commands at init time */
#if WITH_LIB_CONSOLE

/* enable the panic shell if we're being built */
#if !defined(ENABLE_PANIC_SHELL) && PLATFORM_SUPPORTS_PANIC_SHELL
#define ENABLE_PANIC_SHELL 1
#endif

#define STATIC_COMMAND_START static const cmd _cmd_list[] = {

#define STATIC_COMMAND_END(name) }; cmd_block _cmd_block_##name __ALIGNED(sizeof(void *)) __SECTION(".commands") = \
    { NULL, sizeof(_cmd_list) / sizeof(_cmd_list[0]), _cmd_list }

#define STATIC_COMMAND_START_NAMED(name) static const cmd _cmd_list_##name[] = {

#define STATIC_COMMAND_END_NAMED(name) }; cmd_block _cmd_block_##name __ALIGNED(sizeof(void *)) __SECTION(".commands") = \
    { NULL, sizeof(_cmd_list_##name) / sizeof(_cmd_list_##name[0]), _cmd_list_##name }

#define STATIC_COMMAND(command_str, help_str, func) { command_str, help_str, func, CMD_AVAIL_NORMAL },
#define STATIC_COMMAND_MASKED(command_str, help_str, func, availability_mask) { command_str, help_str, func, availability_mask },

#else

/* no command blocks, so null them out */
#define STATIC_COMMAND_START
#define STATIC_COMMAND_END(name)
#define STATIC_COMMAND_START_NAMED(name)
#define STATIC_COMMAND_END_NAMED(name)

#define STATIC_COMMAND(command_str, help_str, func)

#endif

#define COMMAND_BLOCK_INIT_ITEM(cmd_block_ptr, cmd_ptr) {(cmd_block_ptr)->next = NULL; (cmd_block_ptr)->count = 1; (cmd_block_ptr)->list = cmd_ptr;}

/* external api */
int console_init(void);
void console_start(void);
void console_register_commands(cmd_block *block);
int console_run_script(const char *string);
int console_run_script_locked(const char *string); // special case from inside a command
console_cmd console_get_command_handler(const char *command);
void console_abort_script(void);

/* panic shell api */
void panic_shell_start(void);

extern int lastresult;

#endif
