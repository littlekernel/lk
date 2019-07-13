/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <sys/types.h>

/* Queue an ethernet frame for send.
**
** CRC and minimum length padding are handled by the driver.
**
** Data is malloc()'d and ownership is transferred to the ethernet
** device which will free() it once the packet is transmitted.
**
*/
int ethernet_send(void *data, unsigned length);

status_t ethernet_init(void); /* initialize the ethernet device */

