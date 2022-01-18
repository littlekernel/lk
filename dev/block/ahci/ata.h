//
// Copyright (c) 2022 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
#pragma once

#include <stdint.h>
#include <hw/ata.h>

// ata helper routines
inline FIS_REG_H2D ata_cmd_identify() {
    FIS_REG_H2D fis = {};
    fis.fis_type = FIS_TYPE_REG_H2D;
    fis.command = ATA_CMD_IDENTIFY;
    fis.device = 0;
    fis.c = 1;

    return fis;
}

