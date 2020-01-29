#pragma once

#include <arch/vc4_pcb.h>

void fleh_zero(void);
void fleh_misaligned(void);
void fleh_dividebyzero(void);
void fleh_undefinedinstruction(void);
void fleh_forbiddeninstruction(void);
void fleh_illegalmemory(void);
void fleh_buserror(void);
void fleh_floatingpoint(void);
void fleh_isp(void);
void fleh_dummy(void);
void fleh_icache(void);
void fleh_veccore(void);
void fleh_badl2alias(void);
void fleh_breakpoint(void);
void fleh_unknown(void);
void fleh_irq(void);
void fleh_swi(void);
void print_vpu_state(vc4_saved_state_t* pcb);
