#pragma once

#include <platform/bcm28xx/pll.h>

#ifdef __cplusplus
extern "C" {
#endif
uint32_t clk_get_input_freq(uint32_t ctlreg);
uint32_t clk_get_freq(uint32_t divreg, uint32_t ctlreg);
uint32_t get_vpu_per_freq(void);
uint32_t get_uart_base_freq(void);
uint32_t get_pll_freq(enum pll pll);
uint32_t get_pll_chan_freq(enum pll_chan chan);
int measure_clock(int mux);
#ifdef __cplusplus
}
#endif
