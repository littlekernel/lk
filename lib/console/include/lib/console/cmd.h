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
#include <stdalign.h>
#include <sys/types.h>
#include <lk/compiler.h>

__BEGIN_CDECLS

/* Included from top level lk/console_cmd.h when lib/console is built.
 * Provides definitions for how to register a command block to be picked up by
 * the lib/console machinery.
 *
 * Do not include directly.
 */

/* an individual command */
typedef struct {
    const char *cmd_str;
    const char *help_str;
    const console_cmd_func cmd_callback;
    uint8_t availability_mask;
} console_cmd;

/* a block of commands to register */
typedef struct _cmd_block {
    const char *name;
    size_t count;
    const console_cmd *list;
} console_cmd_block;

#define STATIC_COMMAND_START static const console_cmd _cmd_list[] = {

#define STATIC_COMMAND_END(name) }; alignas(console_cmd_block) const console_cmd_block _cmd_block_##name \
    __SECTION("commands") = \
    { #name, sizeof(_cmd_list) / sizeof(_cmd_list[0]), _cmd_list }

/* same as above but with a suffixed name to make the list unique within the file */
#define STATIC_COMMAND_START_NAMED(name) static const console_cmd _cmd_list_##name[] = {

#define STATIC_COMMAND_END_NAMED(name) }; alignas(console_cmd_block) const console_cmd_block _cmd_block_##name \
    __SECTION("commands") = \
    { #name, sizeof(_cmd_list_##name) / sizeof(_cmd_list_##name[0]), _cmd_list_##name }

#define STATIC_COMMAND(command_str, help_str, func) \
    { command_str, help_str, func, CMD_AVAIL_NORMAL },

#define STATIC_COMMAND_MASKED(command_str, help_str, func, availability_mask) \
    { command_str, help_str, func, availability_mask },

__END_CDECLS

