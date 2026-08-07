//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#include <__lk/abort.h>

#include <lk/debug.h>

extern "C" void __lk_cxx_abort(const char *msg) {
    panic("%s\n", msg);
}
