/*
 * Copyright (c) 2008-2009 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>
#include <lk/compiler.h>

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

