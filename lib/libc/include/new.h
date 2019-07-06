/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <sys/types.h>

void *operator new (size_t);
void *operator new (size_t, void *ptr);
void *operator new[](size_t);
void *operator new[](size_t, void *ptr);
void operator delete (void *p);
void operator delete[](void *p);

