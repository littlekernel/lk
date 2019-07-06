/*
 * Copyright (c) 2015 Carlos Pizano-Uribe <cpu@chromium.org>
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#pragma once

#include <lk/compiler.h>

__BEGIN_CDECLS

typedef int (*tftp_callback_t)(void *data, size_t len, void *arg);

int tftp_server_init(void *arg);

int tftp_set_write_client(const char *file_name, tftp_callback_t cb, void *arg);

__END_CDECLS
