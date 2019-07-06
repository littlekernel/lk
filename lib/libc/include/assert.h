/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>
#include <lk/debug.h>

#define ASSERT(x) \
    do { if (unlikely(!(x))) { panic("ASSERT FAILED at (%s:%d): %s\n", __FILE__, __LINE__, #x); } } while (0)

#if LK_DEBUGLEVEL > 1
#define DEBUG_ASSERT(x) \
    do { if (unlikely(!(x))) { panic("DEBUG ASSERT FAILED at (%s:%d): %s\n", __FILE__, __LINE__, #x); } } while (0)
#else
#define DEBUG_ASSERT(x) \
    do { } while(0)
#endif

#define assert(e) DEBUG_ASSERT(e)

#ifndef __cplusplus
#define static_assert(e) STATIC_ASSERT(e)
#endif

