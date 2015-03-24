/*
 * Copyright (c) 2014 Travis Geiselbrecht
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
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

/* recovering boot arguments that were passed */
bool bootargs_are_valid(void);
const char *bootargs_get_command_line(void);
status_t bootargs_get_bootimage_pointer(uint64_t *offset, size_t *len, const char **device);

/* constructing boot arguments */
status_t bootargs_start(void *buf, size_t buf_len);
status_t bootargs_add_command_line(void *buf, size_t buf_len, const char *str);
status_t bootargs_add_bootimage_pointer(void *buf, size_t buf_len, const char *device, uint64_t offset, size_t len);

/* build 4 ulong arguments to pass to a second stage */
void bootargs_generate_lk_arg_values(ulong buf, ulong args[4]);

