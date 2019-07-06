/*
 * Copyright (c) 2016 Brian Swetland
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#pragma once

#include <platform/ti-rf.h>
#include <platform/ti-rf-prop.h>

void radio_init(void);

uint32_t radio_send_cmd(uint32_t cmd);
void radio_wait_cmd(uint16_t *status);



