/*
 * Copyright (c) 2013 Corey Tabaka
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#ifndef __PLATFORM_PCNET_H
#define __PLATFORM_PCNET_H

#include <stdint.h>

struct platform_pcnet_config {
    uint16_t vendor_id;
    uint16_t device_id;
    int index;
};

#endif



