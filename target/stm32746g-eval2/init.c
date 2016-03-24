/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <err.h>
#include <stdlib.h>
#include <debug.h>
#include <trace.h>
#include <target.h>
#include <compiler.h>
#include <lib/gfx.h>
#include <dev/gpio.h>
#include <platform/stm32.h>
#include <platform/sdram.h>
#include <platform/gpio.h>
#include <platform/eth.h>
#include <platform/qspi.h>
#include <platform/n25q512a.h>
#include <target/debugconfig.h>
#include <target/gpioconfig.h>
#include <reg.h>

#if WITH_LIB_MINIP
#include <lib/minip.h>
#endif

extern uint8_t BSP_LCD_Init(void);
extern uint8_t BSP_SRAM_Init(void);

const sdram_config_t target_sdram_config = {
    .bus_width = SDRAM_BUS_WIDTH_32,
    .cas_latency = SDRAM_CAS_LATENCY_3,
    .col_bits_num = SDRAM_COLUMN_BITS_8
};

void target_early_init(void)
{
#if DEBUG_UART == 1
    /* configure usart 1 pins */
    gpio_config(GPIO_USART1_TX, GPIO_STM32_AF | GPIO_STM32_AFn(GPIO_AF7_USART1) | GPIO_PULLUP);
    gpio_config(GPIO_USART1_RX, GPIO_STM32_AF | GPIO_STM32_AFn(GPIO_AF7_USART1) | GPIO_PULLUP);
#else
#error need to configure gpio pins for debug uart
#endif

    /* now that the uart gpios are configured, enable the debug uart */
    stm32_debug_early_init();

    /* initialize external sram */
    BSP_SRAM_Init();

    /* initialize the lcd panel */
    BSP_LCD_Init();
}

void target_init(void)
{
    TRACE_ENTRY;
    stm32_debug_init();

    qspi_flash_init(N25Q512A_FLASH_SIZE);

#if WITH_LIB_MINIP
    uint8_t mac_addr[6];
    gen_random_mac_address(mac_addr);
    eth_init(mac_addr, PHY_DP83848);

    /* start minip */
    minip_set_macaddr(mac_addr);

    uint32_t ip_addr = IPV4(192, 168, 0, 99);
    uint32_t ip_mask = IPV4(255, 255, 255, 0);
    uint32_t ip_gateway = IPV4_NONE;

    minip_init(stm32_eth_send_minip_pkt, NULL, ip_addr, ip_mask, ip_gateway);
#endif

    TRACE_EXIT;
}

#if 0
void target_set_debug_led(unsigned int led, bool on)
{
    switch (led) {
        case 0:
            gpio_set(GPIO_LED0, on);
            break;
        case 1:
            gpio_set(GPIO_LED1, on);
            break;
        case 2:
            gpio_set(GPIO_LED2, on);
            break;
        case 3:
            gpio_set(GPIO_LED3, on);
            break;
    }
}
#endif

/**
  * @brief  Initializes the ETH MSP.
  * @param  heth: ETH handle
  * @retval None
  */
/* called back from the HAL_ETH_Init routine */
void HAL_ETH_MspInit(ETH_HandleTypeDef *heth)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* Enable GPIOs clocks */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOI_CLK_ENABLE();

    /* Ethernet pins configuration ************************************************/
    /*
          ETH_MDIO -------------------------> PA2
          ETH_MDC --------------------------> PC1
          ETH_PPS_OUT ----------------------> PB5
          ETH_MII_RXD2 ---------------------> PH6
          ETH_MII_RXD3 ---------------------> PH7
          ETH_MII_TX_CLK -------------------> PC3
          ETH_MII_TXD2 ---------------------> PC2
          ETH_MII_TXD3 ---------------------> PE2
          ETH_MII_RX_CLK -------------------> PA1
          ETH_MII_RX_DV --------------------> PA7
          ETH_MII_RXD0 ---------------------> PC4
          ETH_MII_RXD1 ---------------------> PC5
          ETH_MII_TX_EN --------------------> PG11
          ETH_MII_TXD0 ---------------------> PG13
          ETH_MII_TXD1 ---------------------> PG14
          ETH_MII_RX_ER --------------------> PI10 (not configured)
          ETH_MII_CRS ----------------------> PA0  (not configured)
          ETH_MII_COL ----------------------> PH3  (not configured)
    */

    /* Configure PA1, PA2 and PA7 */
    GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStructure.Pull = GPIO_NOPULL;
    GPIO_InitStructure.Alternate = GPIO_AF11_ETH;
    GPIO_InitStructure.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_7;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* Note : ETH_MDIO is connected to PA2 which is shared with other signals like SAI2_SCKB.
       By default on STM32756G-EVAL board, PA2 is connected to SAI2_SCKB, so to connect PA2 to ETH_MDIO :
      - unsolder bridge SB24 (SAI2_CKB)
      - solder bridge SB5 (ETH_MDIO) */

    /* Configure PB5 */
    GPIO_InitStructure.Pin = GPIO_PIN_5;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* Configure PE2 */
    GPIO_InitStructure.Pin = GPIO_PIN_2;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStructure);

    /* Configure PC1, PC2, PC3, PC4 and PC5 */
    GPIO_InitStructure.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);

    /* Note : ETH_MDC is connected to PC1 which is shared with other signals like SAI1_SDA.
       By default on STM32756G-EVAL board, PC1 is connected to SAI1_SDA, so to connect PC1 to ETH_MDC :
      - unsolder bridge SB22 (SAI1_SDA)
      - solder bridge SB33 (ETH_MDC) */

    /* Configure PG11, PG14 and PG13 */
    GPIO_InitStructure.Pin =  GPIO_PIN_11 | GPIO_PIN_13 | GPIO_PIN_14;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStructure);

    /* Configure PH6, PH7 */
    GPIO_InitStructure.Pin =  GPIO_PIN_6 | GPIO_PIN_7;
    HAL_GPIO_Init(GPIOH, &GPIO_InitStructure);

    /* Configure PA0
    GPIO_InitStructure.Pin =  GPIO_PIN_0;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);

    Note: Ethernet Full duplex mode works properly in the default setting
    (which MII_CRS is not connected to PA0 of STM32F756NGH6) because PA0 is shared
    with MC_ENA.
    If Half duplex mode is needed, uncomment PA0 configuration code source (above
    the note) and close the SB36 solder bridge of the STM32756G-EVAL board .
    */

    /* Configure PH3
    GPIO_InitStructure.Pin =  GPIO_PIN_3;
    HAL_GPIO_Init(GPIOH, &GPIO_InitStructure);

    Note: Ethernet Full duplex mode works properly in the default setting
    (which MII_COL is not connected to PH3 of STM32F756NGH6) because PH3 is shared
    with SDRAM chip select SDNE0.
    If Half duplex mode is needed, uncomment PH3 configuration code source (above
    the note) and close SB47 solder bridge of the STM32756G-EVAL board.
    */

    /* Configure PI10
    GPIO_InitStructure.Pin = GPIO_PIN_10;
    HAL_GPIO_Init(GPIOI, &GPIO_InitStructure);

    Note: Ethernet works properly in the default setting (which RX_ER is not
    connected to PI10 of STM32F756NGH6) because PI10 is shared with data signal
    of SDRAM.
    If RX_ER signal is needed, uncomment PI10 configuration code source (above
    the note) then remove R248 and solder SB9 of the STM32756G-EVAL board.
    */
}

/**
  * @brief  Initializes SDRAM GPIO.
  * @retval None
  */
/* called back from BSP_SDRAM_Init */
void stm_sdram_GPIO_init(void)
{
    GPIO_InitTypeDef gpio_init_structure;

    /* Enable GPIOs clock */
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOI_CLK_ENABLE();

    /* Common GPIO configuration */
    gpio_init_structure.Mode      = GPIO_MODE_AF_PP;
    gpio_init_structure.Pull      = GPIO_PULLUP;
    gpio_init_structure.Speed     = GPIO_SPEED_FAST;
    gpio_init_structure.Alternate = GPIO_AF12_FMC;

    /* GPIOD configuration */
    gpio_init_structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_8| GPIO_PIN_9 | GPIO_PIN_10 |\
                                GPIO_PIN_14 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOD, &gpio_init_structure);

    /* GPIOE configuration */
    gpio_init_structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_7| GPIO_PIN_8 | GPIO_PIN_9 |\
                                GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 |\
                                GPIO_PIN_15;
    HAL_GPIO_Init(GPIOE, &gpio_init_structure);

    /* GPIOF configuration */
    gpio_init_structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2| GPIO_PIN_3 | GPIO_PIN_4 |\
                                GPIO_PIN_5 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 |\
                                GPIO_PIN_15;
    HAL_GPIO_Init(GPIOF, &gpio_init_structure);

    /* GPIOG configuration */
    gpio_init_structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4| GPIO_PIN_5 | GPIO_PIN_8 |\
                                GPIO_PIN_15;
    HAL_GPIO_Init(GPIOG, &gpio_init_structure);

    /* GPIOH configuration */
    gpio_init_structure.Pin   = GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_5 | GPIO_PIN_8 | GPIO_PIN_9 |\
                                GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 |\
                                GPIO_PIN_15;
    HAL_GPIO_Init(GPIOH, &gpio_init_structure);

    /* GPIOI configuration */
    gpio_init_structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 |\
                                GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_9 | GPIO_PIN_10;
    HAL_GPIO_Init(GPIOI, &gpio_init_structure);
}

void HAL_QSPI_MspInit(QSPI_HandleTypeDef *hqspi)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    /*##-1- Enable peripherals and GPIO Clocks #################################*/
    /* Enable GPIO clocks */
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();

    /*##-2- Configure peripheral GPIO ##########################################*/
    /* QSPI CS GPIO pin configuration  */
    GPIO_InitStruct.Pin       = GPIO_PIN_6;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_QUADSPI;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* QSPI CLK GPIO pin configuration  */
    GPIO_InitStruct.Pin       = GPIO_PIN_2;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Alternate = GPIO_AF9_QUADSPI;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* QSPI D0 GPIO pin configuration  */
    GPIO_InitStruct.Pin       = GPIO_PIN_8;
    GPIO_InitStruct.Alternate = GPIO_AF10_QUADSPI;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    /* QSPI D1 GPIO pin configuration  */
    GPIO_InitStruct.Pin       = GPIO_PIN_9;
    GPIO_InitStruct.Alternate = GPIO_AF10_QUADSPI;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    /* QSPI D2 GPIO pin configuration  */
    GPIO_InitStruct.Pin       = GPIO_PIN_7;
    GPIO_InitStruct.Alternate = GPIO_AF9_QUADSPI;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    /* QSPI D3 GPIO pin configuration  */
    GPIO_InitStruct.Pin       = GPIO_PIN_6;
    GPIO_InitStruct.Alternate = GPIO_AF9_QUADSPI;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
}

