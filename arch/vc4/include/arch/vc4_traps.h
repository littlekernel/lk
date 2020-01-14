#pragma once

#include <arch/vc4_pcb.h>

void fleh_zero(void);
void fleh_misaligned(void);
void fleh_dividebyzero();
void fleh_undefinedinstruction();
void fleh_forbiddeninstruction();
void fleh_illegalmemory();
void fleh_buserror();
void fleh_floatingpoint();
void fleh_isp();
void fleh_dummy();
void fleh_icache();
void fleh_veccore();
void fleh_badl2alias();
void fleh_breakpoint();
void fleh_unknown();
void fleh_irq();
void print_vpu_state(vc4_saved_state_t* pcb);
