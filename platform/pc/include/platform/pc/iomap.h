/*
 * Copyright (c) 2009 Corey Tabaka
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

/* i8253/i8254 programmable interval timer registers */
#define I8253_CONTROL_REG   0x43
#define I8253_DATA_REG      0x40

/* i8042 keyboard controller registers */
#define I8042_COMMAND_REG   0x64
#define I8042_STATUS_REG    0x64
#define I8042_DATA_REG      0x60

/* CMOS/RTC registers */
#define CMOS_CONTROL_REG    0x70
#define CMOS_DATA_REG       0x71

/* CGA registers */
#define CGA_INDEX_REG       0x3d4
#define CGA_DATA_REG        0x3d5

/* COM (serial) ports */
#define COM1_REG            0x3f8
#define COM2_REG            0x2f8
#define COM3_REG            0x3e8
#define COM4_REG            0x2e8

