/*
 * Copyright (c) 2025 Linaro Ltd.
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 * TODO: License
 */

#pragma once

/*
 * The normal build directly includes this before the source is processed, with bindgen, it is
 * easiest to just include it.
 */
#include "config.h"

#ifndef LK
    #error "config.h did not properly add defines needed"
#endif

#include <stdio.h>
#include <malloc.h>

#include <lk/init.h>
#include <lk/debug.h>
#include <lib/cbuf.h>

#include <platform/interrupts.h>

// #include "error.h"
