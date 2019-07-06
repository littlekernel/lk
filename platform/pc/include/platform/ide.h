/*
 * Copyright (c) 2013 Corey Tabaka
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

struct platform_ide_config {
    int legacy_index; // 0x80, 0x81 for pci detection channel 0 and 1, 0 or 1 for legacy ISA IDE
};

