//
// Copyright (c) 2021 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#pragma once

#include <stdint.h>

// list of known 8086:x e1000/e1000e devices to match against
struct e1000_id_features {
    uint16_t id;
    bool e1000e;
};

constexpr e1000_id_features e1000_ids[] = {
    { 0x100c, false }, // 82544GC QEMU 'e1000-82544gc'
    { 0x100e, false }, // 82540EM QEMU 'e1000'
    { 0x100f, false }, // 82545EM QEMU 'e1000-82544em'
    { 0x10d3, true }, // 82574L  QEMU 'e1000e'

    // i210/i211 family
    { 0x1533, true }, // i210

    // i219 family (PCH integrated, handled as e1000e)
    { 0x0d4d, true }, // Platform GbE Controller (Consumer), i219-class
    { 0x0d53, true }, // i219-LM
    { 0x0d55, true }, // i219-V
    { 0x0dc5, true }, // i219-LM
    { 0x0dc6, true }, // i219-V
    { 0x0dc7, true }, // i219-LM
    { 0x0dc8, true }, // i219-V
    { 0x156f, true }, // i219-LM
    { 0x1570, true }, // i219-V
    { 0x15b7, true }, // i219-LM
    { 0x15b8, true }, // i219-V
    { 0x15b9, true }, // i219-LM
    { 0x15bb, true }, // i219-LM
    { 0x15bc, true }, // i219-V
    { 0x15bd, true }, // i219-LM
    { 0x15be, true }, // i219-V
    { 0x15d6, true }, // i219-V
    { 0x15d7, true }, // i219-LM
    { 0x15d8, true }, // i219-V
    { 0x15df, true }, // i219-LM
    { 0x15e0, true }, // i219-V
    { 0x15e1, true }, // i219-LM
    { 0x15e2, true }, // i219-V
    { 0x15e3, true }, // i219-LM
    { 0x15f4, true }, // i219-LM
    { 0x15f5, true }, // i219-V
    { 0x15fb, true }, // i219-LM
    { 0x15fc, true }, // i219-V
    { 0x1a1c, true }, // i219-LM (17)
    { 0x1a1d, true }, // i219-V
    { 0x1a1e, true }, // i219-LM (16)
    { 0x1a1f, true }, // i219-V  (16)
    { 0x550a, true }, // i219-LM
    { 0x550b, true }, // i219-LM
    { 0x550e, true }, // i219-LM
    { 0x550f, true }, // i219-V
    { 0x5510, true }, // i219-LM
    { 0x5511, true }, // i219-V
    { 0x57a0, true }, // i219-LM
    { 0x57a1, true }, // i219-V
    { 0x57b3, true }, // i219-LM
    { 0x57b4, true }, // i219-V
    { 0x57b5, true }, // i219-LM
    { 0x57b6, true }, // i219-V
    { 0x57b7, true }, // i219-LM
    { 0x57b8, true }, // i219-V
    { 0x57b9, true }, // i219-LM
    { 0x57ba, true }, // i219-V
};
