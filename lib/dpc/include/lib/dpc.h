/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/list.h>
#include <sys/types.h>

typedef void (*dpc_callback)(void *arg);

#define DPC_FLAG_NORESCHED 0x1

status_t dpc_queue(dpc_callback, void *arg, uint flags);

