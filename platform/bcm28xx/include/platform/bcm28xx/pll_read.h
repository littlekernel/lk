#pragma once

#ifdef __cplusplus
extern "C" {
#endif
uint32_t clk_get_input_freq(uint32_t ctlreg);
uint32_t clk_get_freq(uint32_t divreg, uint32_t ctlreg);
uint32_t get_vpu_per_freq(void);
uint32_t get_uart_base_freq(void);
uint32_t plla(void);
uint32_t pllb(void);
uint32_t pllc(void);
uint32_t plld(void);
uint32_t pllh(void);
extern uint32_t xtal_freq;
#ifdef __cplusplus
}
#endif
