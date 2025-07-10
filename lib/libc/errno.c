/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <errno.h>
#include <kernel/thread.h>

int *__geterrno(void) {
    return (int*)tls_get(TLS_ENTRY_ERRNO);
}

