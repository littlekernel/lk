// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fbl/string_buffer.h>

#include <stdio.h>
#include <string.h>

namespace fbl {
namespace internal {

size_t StringBufferAppendPrintf(char* dest, size_t remaining,
                                const char* format, va_list ap) {
    if (remaining == 0u) {
        return 0u;
    }
    int count = vsnprintf(dest, remaining + 1u, format, ap);
    if (count < 0) {
        return 0u;
    }
    size_t length = static_cast<size_t>(count);
    return length >= remaining ? remaining : length;
}

} // namespace internal
} // namespace fbl
