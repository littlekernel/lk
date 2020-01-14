#pragma once

#ifdef __cplusplus
extern "C" {
#endif
uint32_t clk_get_input_freq(volatile uint32_t *ctlreg);
uint32_t clk_get_freq(volatile uint32_t *divreg, volatile uint32_t *ctlreg);
uint32_t get_vpu_per_freq();
uint32_t get_uart_base_freq();
uint32_t plla();
uint32_t pllb();
uint32_t pllc();
uint32_t plld();
uint32_t pllh();
extern uint32_t xtal_freq;
#ifdef __cplusplus
}
#endif
