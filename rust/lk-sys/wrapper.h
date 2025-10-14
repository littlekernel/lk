/*
 * Copyright (c) 2025 Linaro Ltd. All rights reserved
 *
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
