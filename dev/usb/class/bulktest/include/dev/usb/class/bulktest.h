/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <sys/types.h>
#include <dev/usbc.h>
#include <dev/usb.h>

status_t usb_class_bulktest_init(uint interface_num, ep_t epin, ep_t epout);

