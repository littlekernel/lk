/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <string.h>
#include <sys/types.h>

char const *
strerror(int errnum) {
    if (errnum < 0) {
        return "General Error";
    } else {
        return "No Error";
    }
}

