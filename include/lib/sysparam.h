/*
 * Copyright (c) 2013, Google, Inc. All rights reserved
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

#include <stdbool.h>
#include <sys/types.h>
#include <lib/bio.h>

#ifndef SYSPARAM_ALLOW_WRITE
#define SYSPARAM_ALLOW_WRITE 0
#endif

status_t sysparam_scan(bdev_t *bdev, off_t offset, size_t len);
status_t sysparam_reload(void);

void sysparam_dump(bool show_all);

ssize_t sysparam_length(const char *name);
ssize_t sysparam_read(const char *name, void *data, size_t len);
status_t sysparam_get_ptr(const char *name, const void **ptr, size_t *len);

#if SYSPARAM_ALLOW_WRITE
status_t sysparam_add(const char *name, const void *value, size_t len);
status_t sysparam_remove(const char *name);
status_t sysparam_lock(const char *name);
status_t sysparam_write(void);
#endif

