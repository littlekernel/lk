/*
 * Copyright 2016 Google Inc. All Rights Reserved.
 * Author: gkalsi@google.com (Gurjant Kalsi)
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#ifndef APP_MOOT_USB_H_
#define APP_MOOT_USB_H_

#include <stdbool.h>

// Initialize the USB stack / USB boot mechanisms.
void init_usb_boot(void);

// Allow the USB device to interrupt the boot sequence.
void attempt_usb_boot(void);

#endif  // APP_MOOT_USB_H_