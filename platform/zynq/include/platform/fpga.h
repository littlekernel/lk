
#pragma once

#include <sys/types.h>

void zynq_program_fpga(u32 physaddr, u32 length);
void zynq_reset_fpga(void);

