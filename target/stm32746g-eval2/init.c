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
#include <platform/gpio.h>
#include <target/debugconfig.h>
#include <target/gpioconfig.h>
#include <reg.h>

#if WITH_LIB_MINIP
#include <lib/minip.h>
#endif

extern uint8_t BSP_SDRAM_Init(void);
extern uint8_t BSP_LCD_Init(void);
extern uint8_t BSP_SRAM_Init(void);

static void MPU_RegionConfig(void);

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

    /* initialize sdram */
    BSP_SDRAM_Init();

    /* initialize external sram */
    BSP_SRAM_Init();

    /* initialize the mpu */
    MPU_RegionConfig();

    /* initialize the lcd panel */
    BSP_LCD_Init();
}

/**
  * @brief  Configures the main MPU regions.
  * @param  None
  * @retval None
  */
static void MPU_RegionConfig(void)
{
    MPU_Region_InitTypeDef MPU_InitStruct;

    /* Disable MPU */
    HAL_MPU_Disable();

    uint region_num = 0;

#if 1
    // SDRAM
    MPU_InitStruct.Enable = MPU_REGION_ENABLE;
    MPU_InitStruct.BaseAddress = SDRAM_BASE;
    MPU_InitStruct.Size = MPU_REGION_SIZE_32MB;
    MPU_InitStruct.AccessPermission = MPU_REGION_PRIV_RW;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
    MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
    MPU_InitStruct.Number = region_num++;
    MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
    MPU_InitStruct.SubRegionDisable = 0x00;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;

    HAL_MPU_ConfigRegion(&MPU_InitStruct);
#endif

    // SRAM
#if 1
    MPU_InitStruct.Enable = MPU_REGION_ENABLE;
    MPU_InitStruct.BaseAddress = EXT_SRAM_BASE;
    MPU_InitStruct.Size = MPU_REGION_SIZE_2MB;
    MPU_InitStruct.AccessPermission = MPU_REGION_PRIV_RW;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
    MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
    MPU_InitStruct.Number = region_num++;
    MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
    MPU_InitStruct.SubRegionDisable = 0x00;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;

    HAL_MPU_ConfigRegion(&MPU_InitStruct);
#endif

    /* don't have to enable these if we let the MPU use default permissions for stuff other than SDRAM */
#if 0
    /* Configure RAM region as Region N°0, 1MB of size and R/W region */
    MPU_InitStruct.Enable = MPU_REGION_ENABLE;
    MPU_InitStruct.BaseAddress = 0x20000000;
    MPU_InitStruct.Size = MPU_REGION_SIZE_1MB;
    MPU_InitStruct.AccessPermission = MPU_REGION_PRIV_RW;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
    MPU_InitStruct.Number = 0;
    MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
    MPU_InitStruct.SubRegionDisable = 0x00;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;

    HAL_MPU_ConfigRegion(&MPU_InitStruct);

    /* Configure FLASH region as REGION N°1, 1MB of size and R/W region */
    MPU_InitStruct.BaseAddress = 0x02000000; // FLASH_ADDRESS_START;
    MPU_InitStruct.Size = MPU_REGION_SIZE_1MB; // FLASH_SIZE;
    MPU_InitStruct.Number = 1;

    HAL_MPU_ConfigRegion(&MPU_InitStruct);

    /* Configure Peripheral region as REGION N°2, 0.5GB of size, R/W and Execute
    Never region */
    MPU_InitStruct.BaseAddress = 0x40000000; // PERIPH_ADDRESS_START;
    MPU_InitStruct.Size = MPU_REGION_SIZE_512KB; // PERIPH_SIZE;
    MPU_InitStruct.Number = 2; // PERIPH_REGION_NUMBER;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;

    HAL_MPU_ConfigRegion(&MPU_InitStruct);
#endif

    /* Enable MPU */
    HAL_MPU_Enable(MPU_HFNMI_PRIVDEF);
}


void target_init(void)
{
    TRACE_ENTRY;
    stm32_debug_init();

#if WITH_LIB_MINIP
    // make up a mac address
    uint8_t mac_addr[6];
    for (size_t i = 0; i < sizeof(mac_addr); i++) {
        mac_addr[i] = rand() & 0xff;
    }

    /* unicast and locally administered */
    mac_addr[0] &= ~(1<<0);
    mac_addr[0] |= (1<<1);

    eth_init(mac_addr);

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



