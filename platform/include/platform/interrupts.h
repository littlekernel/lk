/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>
#include <sys/types.h>

__BEGIN_CDECLS

/* Routines implemented by the platform or system specific interrupt controller
 * to allow for installation and masking/unmask of interrupt vectors.
 *
 * Some platforms do not allow for dynamic registration.
 */
status_t mask_interrupt(unsigned int vector);
status_t unmask_interrupt(unsigned int vector);

typedef enum handler_return (*int_handler)(void *arg);

void register_int_handler(unsigned int vector, int_handler handler, void *arg);

__END_CDECLS
