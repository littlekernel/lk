/*
 * Copyright (c) 2025 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <stdio.h>
#include <inttypes.h>

#include "printf_tests_float_vec.h"

// Host program that will print the floating point test vector for manual comparison.

int main(int argc, char **argv) {
    printf("floating point printf tests\n");

    for (const auto &vec : float_test_vec) {
        printf("%" PRIx64 " %f %F %a %A\n", \
                vec.i, \
                vec.d, \
                vec.d, \
                vec.d, \
                vec.d);
    }

    return 0;
}

