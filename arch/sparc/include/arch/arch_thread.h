//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
//
#pragma once

#include <sys/types.h>

struct sparc_context_switch_frame {
    uint32_t sp;
    uint32_t pc;
};

struct arch_thread {
    struct sparc_context_switch_frame cs_frame;
};

void sparc_context_switch(struct sparc_context_switch_frame *oldcs, struct sparc_context_switch_frame *newcs);
