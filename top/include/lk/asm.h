/*
 * Copyright (c) 2008-2013 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#define FUNCTION(x) .global x; .type x,STT_FUNC; x:
#define DATA(x) .global x; .type x,STT_OBJECT; x:

#define LOCAL_FUNCTION(x) .type x,STT_FUNC; x:
#define LOCAL_DATA(x) .type x,STT_OBJECT; x:

#define END_FUNCTION(x) .size x, . - x
#define END_DATA(x) .size x, . - x
