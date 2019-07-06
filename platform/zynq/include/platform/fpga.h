/*
 * Copyright (c) 2014 Brian Swetland
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <sys/types.h>
#include <stdbool.h>

status_t zynq_program_fpga(paddr_t physaddr, size_t length);
bool zync_fpga_config_done(void);
void zynq_reset_fpga(void);

