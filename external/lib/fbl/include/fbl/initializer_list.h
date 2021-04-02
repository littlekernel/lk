// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//
// Provides fbl::initializer_list<T>.
// Unfortunately the C++ compiler hardcodes a dependency on std::initializer_list<T>.
// We use the C++ standard library definition when available, otherwise we
// use the one offered by the Zircon toolchain.
//

#pragma once

#ifdef ZIRCON_TOOLCHAIN
// Use the implementation supplied by the Zircon toolchain when we are
// compiling without the C++ standard library.
#include <zircon/initializer_list.h>
#else
// Use the implementation supplied by the C++ standard library.
#include <initializer_list>
#endif // ZIRCON_TOOLCHAIN

// Expose std::initializer_list<T> as fbl::initializer_list<T>.
namespace fbl {
using std::initializer_list;
}
