// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fbl/string_piece.h>

#include <fbl/algorithm.h>

namespace fbl {

int StringPiece::compare(const StringPiece& other) const {
    size_t len = min(length_, other.length_);
    int retval = memcmp(data_, other.data_, len);
    if (retval == 0) {
        if (length_ == other.length_) {
            return 0;
        }
        return length_ < other.length_ ? -1 : 1;
    }
    return retval;
}

bool operator==(const StringPiece& lhs, const StringPiece& rhs) {
    return lhs.length() == rhs.length() &&
           memcmp(lhs.data(), rhs.data(), lhs.length()) == 0;
}

} // namespace fbl
