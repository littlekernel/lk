/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <string.h>
#include <sys/types.h>

char *
strerror(int errnum) {
    /* The C standard requires a non-const return type for backwards compat. */
    if (errnum < 0) {
        return (char *)"General Error";
    } else {
        return (char *)"No Error";
    }
}

