/*
 * Copyright 2018 The Fuchsia Authors. All Rights Reserved.
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#pragma once

#include <dev/usb/class/cdcserial.h>

/* This needs to be called before app_init. */
void cdctest_setup(cdcserial_channel_t *chan);
