/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <errno.h>

/* completely un-threadsafe implementation of errno */
/* TODO: pull from kernel TLS or some other thread local storage */
static int _errno;

int *__geterrno(void) {
    return &_errno;
}

