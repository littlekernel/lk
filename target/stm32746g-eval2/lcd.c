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
/*
 * COPYRIGHT(c) 2015 STMicroelectronics
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of STMicroelectronics nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */

#include <assert.h>
#include <err.h>
#include <debug.h>
#include <trace.h>
#include <target.h>
#include <compiler.h>
#include <string.h>
#include <dev/gpio.h>
#include <dev/display.h>
#include <arch/ops.h>
#include <platform/stm32.h>

/*
 * lcd initialization sequence, taken from
 * STM32Cube_FW_F7_V1.1.0/Drivers/BSP/STM32756G_EVAL/stm32756g_eval_lcd.[ch]
 */

/**
  * @brief  AMPIRE640480 Size
  */
#define  AMPIRE640480_WIDTH    ((uint16_t)640)             /* LCD PIXEL WIDTH            */
#define  AMPIRE640480_HEIGHT   ((uint16_t)480)             /* LCD PIXEL HEIGHT           */

/**
  * @brief  AMPIRE640480 Timing
  */
#define  AMPIRE640480_HSYNC            ((uint16_t)30)      /* Horizontal synchronization */
#define  AMPIRE640480_HBP              ((uint16_t)114)     /* Horizontal back porch      */
#define  AMPIRE640480_HFP              ((uint16_t)16)      /* Horizontal front porch     */
#define  AMPIRE640480_VSYNC            ((uint16_t)3)       /* Vertical synchronization   */
#define  AMPIRE640480_VBP              ((uint16_t)32)      /* Vertical back porch        */
#define  AMPIRE640480_VFP              ((uint16_t)10)      /* Vertical front porch       */

/**
  * @brief  AMPIRE640480 frequency divider
  */
#define  AMPIRE640480_FREQUENCY_DIVIDER     3              /* LCD Frequency divider      */

/** @defgroup STM32756G_EVAL_LCD_Exported_Constants
  * @{
  */
#define MAX_LAYER_NUMBER       ((uint32_t)2)

#define LCD_LayerCfgTypeDef    LTDC_LayerCfgTypeDef

#define LTDC_ACTIVE_LAYER        ((uint32_t)1) /* Layer 1 */
/**
  * @brief  LCD status structure definition
  */
#define LCD_OK                 ((uint8_t)0x00)
#define LCD_ERROR              ((uint8_t)0x01)
#define LCD_TIMEOUT            ((uint8_t)0x02)

/**
  * @brief  LCD FB_StartAddress
  */
#define LCD_FB_START_ADDRESS       ((uint32_t)SDRAM_BASE)
//#define LCD_FB_START_ADDRESS       ((uint32_t)EXT_SRAM_BASE)

//#define LCD_PIXEL_FORMAT LTDC_PIXEL_FORMAT_ARGB888
#define LCD_PIXEL_FORMAT LTDC_PIXEL_FORMAT_RGB565

static LTDC_HandleTypeDef  hLtdcEval;

/* Default LCD configuration with LCD Layer 1 */
static uint32_t            ActiveLayer = 0;

/**
  * @brief  Gets the LCD X size.
  * @retval Used LCD X size
  */
uint32_t BSP_LCD_GetXSize(void)
{
    return hLtdcEval.LayerCfg[ActiveLayer].ImageWidth;
}

/**
  * @brief  Gets the LCD Y size.
  * @retval Used LCD Y size
  */
uint32_t BSP_LCD_GetYSize(void)
{
    return hLtdcEval.LayerCfg[ActiveLayer].ImageHeight;
}

/**
  * @brief  Set the LCD X size.
  * @param  imageWidthPixels : image width in pixels unit
  * @retval None
  */
void BSP_LCD_SetXSize(uint32_t imageWidthPixels)
{
    hLtdcEval.LayerCfg[ActiveLayer].ImageWidth = imageWidthPixels;
}

/**
  * @brief  Set the LCD Y size.
  * @param  imageHeightPixels : image height in lines unit
  * @retval None
  */
void BSP_LCD_SetYSize(uint32_t imageHeightPixels)
{
    hLtdcEval.LayerCfg[ActiveLayer].ImageHeight = imageHeightPixels;
}

static size_t BSP_LCD_PixelSize(void)
{
    return (hLtdcEval.LayerCfg[ActiveLayer].PixelFormat == LTDC_PIXEL_FORMAT_ARGB8888) ? 4 : 2;
}

/**
  * @brief  Initializes the LCD layers.
  * @param  LayerIndex: Layer foreground or background
  * @param  FB_Address: Layer frame buffer
  * @retval None
  */
void BSP_LCD_LayerDefaultInit(uint16_t LayerIndex, uint32_t FB_Address)
{
    LCD_LayerCfgTypeDef  layer_cfg;

    /* Layer Init */
    layer_cfg.WindowX0 = 0;
    layer_cfg.WindowX1 = BSP_LCD_GetXSize();
    layer_cfg.WindowY0 = 0;
    layer_cfg.WindowY1 = BSP_LCD_GetYSize();
    layer_cfg.PixelFormat = LCD_PIXEL_FORMAT;
    layer_cfg.FBStartAdress = FB_Address;
    layer_cfg.Alpha = 255;
    layer_cfg.Alpha0 = 0;
    layer_cfg.Backcolor.Blue = 0;
    layer_cfg.Backcolor.Green = 0;
    layer_cfg.Backcolor.Red = 0;
    layer_cfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
    layer_cfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
    layer_cfg.ImageWidth = BSP_LCD_GetXSize();
    layer_cfg.ImageHeight = BSP_LCD_GetYSize();

    HAL_LTDC_ConfigLayer(&hLtdcEval, &layer_cfg, LayerIndex);
}


/**
  * @brief  Selects the LCD Layer.
  * @param  LayerIndex: Layer foreground or background
  * @retval None
  */
void BSP_LCD_SelectLayer(uint32_t LayerIndex)
{
    ActiveLayer = LayerIndex;
}

/**
  * @brief  Sets an LCD Layer visible
  * @param  LayerIndex: Visible Layer
  * @param  State: New state of the specified layer
  *          This parameter can be one of the following values:
  *            @arg  ENABLE
  *            @arg  DISABLE
  * @retval None
  */
void BSP_LCD_SetLayerVisible(uint32_t LayerIndex, FunctionalState State)
{
    if (State == ENABLE) {
        __HAL_LTDC_LAYER_ENABLE(&hLtdcEval, LayerIndex);
    } else {
        __HAL_LTDC_LAYER_DISABLE(&hLtdcEval, LayerIndex);
    }
    __HAL_LTDC_RELOAD_CONFIG(&hLtdcEval);
}

/**
  * @brief  Configures the transparency.
  * @param  LayerIndex: Layer foreground or background.
  * @param  Transparency: Transparency
  *           This parameter must be a number between Min_Data = 0x00 and Max_Data = 0xFF
  * @retval None
  */
void BSP_LCD_SetTransparency(uint32_t LayerIndex, uint8_t Transparency)
{
    HAL_LTDC_SetAlpha(&hLtdcEval, Transparency, LayerIndex);
}

/**
  * @brief  Sets an LCD layer frame buffer address.
  * @param  LayerIndex: Layer foreground or background
  * @param  Address: New LCD frame buffer value
  * @retval None
  */
void BSP_LCD_SetLayerAddress(uint32_t LayerIndex, uint32_t Address)
{
    HAL_LTDC_SetAddress(&hLtdcEval, Address, LayerIndex);
}

/**
  * @brief  Enables the display.
  * @retval None
  */
void BSP_LCD_DisplayOn(void)
{
    /* Display On */
    __HAL_LTDC_ENABLE(&hLtdcEval);
}

/**
  * @brief  Disables the display.
  * @retval None
  */
void BSP_LCD_DisplayOff(void)
{
    /* Display Off */
    __HAL_LTDC_DISABLE(&hLtdcEval);
}

/**
  * @brief  Initializes the LTDC MSP.
  * @param  hltdc: LTDC handle
  * @retval None
  */
static void BSP_LCD_MspInit(LTDC_HandleTypeDef *hltdc, void *Params)
{
    GPIO_InitTypeDef gpio_init_structure;

    /* Enable the LTDC and DMA2D clocks */
    __HAL_RCC_LTDC_CLK_ENABLE();
    __HAL_RCC_DMA2D_CLK_ENABLE();

    /* Enable GPIOs clock */
    __HAL_RCC_GPIOI_CLK_ENABLE();
    __HAL_RCC_GPIOJ_CLK_ENABLE();
    __HAL_RCC_GPIOK_CLK_ENABLE();

    /*** LTDC Pins configuration ***/
    /* GPIOI configuration */
    gpio_init_structure.Pin       = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    gpio_init_structure.Mode      = GPIO_MODE_AF_PP;
    gpio_init_structure.Pull      = GPIO_NOPULL;
    gpio_init_structure.Speed     = GPIO_SPEED_FAST;
    gpio_init_structure.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOI, &gpio_init_structure);

    /* GPIOJ configuration */
    gpio_init_structure.Pin       = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | \
                                    GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | \
                                    GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | \
                                    GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    gpio_init_structure.Mode      = GPIO_MODE_AF_PP;
    gpio_init_structure.Pull      = GPIO_NOPULL;
    gpio_init_structure.Speed     = GPIO_SPEED_FAST;
    gpio_init_structure.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOJ, &gpio_init_structure);

    /* GPIOK configuration */
    gpio_init_structure.Pin       = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | \
                                    GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    gpio_init_structure.Mode      = GPIO_MODE_AF_PP;
    gpio_init_structure.Pull      = GPIO_NOPULL;
    gpio_init_structure.Speed     = GPIO_SPEED_FAST;
    gpio_init_structure.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOK, &gpio_init_structure);
}

/**
  * @brief  Clock Config.
  * @param  hltdc: LTDC handle
  * @note   This API is called by BSP_LCD_Init()
  *         Being __weak it can be overwritten by the application
  * @retval None
  */
static void BSP_LCD_ClockConfig(LTDC_HandleTypeDef *hltdc, void *Params)
{
    static RCC_PeriphCLKInitTypeDef  periph_clk_init_struct;

    /* AMPIRE640480 LCD clock configuration */
    /* PLLSAI_VCO Input = HSE_VALUE/PLL_M = 1 Mhz */
    /* PLLSAI_VCO Output = PLLSAI_VCO Input * PLLSAIN = 151 Mhz */
    /* PLLLCDCLK = PLLSAI_VCO Output/PLLSAIR = 151/3 = 50.3 Mhz */
    /* LTDC clock frequency = PLLLCDCLK / LTDC_PLLSAI_DIVR_2 = 50.3/2 = 25.1 Mhz */
    periph_clk_init_struct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
    periph_clk_init_struct.PLLSAI.PLLSAIN = 151;
    periph_clk_init_struct.PLLSAI.PLLSAIR = AMPIRE640480_FREQUENCY_DIVIDER;
    periph_clk_init_struct.PLLSAIDivR = RCC_PLLSAIDIVR_2;
    HAL_RCCEx_PeriphCLKConfig(&periph_clk_init_struct);
}

/**
  * @brief  Initializes the LCD.
  * @retval LCD state
  */
uint8_t BSP_LCD_Init(void)
{
    /* Select the used LCD */
    /* The LCD AMPIRE 640x480 is selected */
    /* Timing configuration */
    hLtdcEval.Init.HorizontalSync = (AMPIRE640480_HSYNC - 1);
    hLtdcEval.Init.VerticalSync = (AMPIRE640480_VSYNC - 1);
    hLtdcEval.Init.AccumulatedHBP = (AMPIRE640480_HSYNC + AMPIRE640480_HBP - 1);
    hLtdcEval.Init.AccumulatedVBP = (AMPIRE640480_VSYNC + AMPIRE640480_VBP - 1);
    hLtdcEval.Init.AccumulatedActiveH = (AMPIRE640480_HEIGHT + AMPIRE640480_VSYNC + AMPIRE640480_VBP - 1);
    hLtdcEval.Init.AccumulatedActiveW = (AMPIRE640480_WIDTH + AMPIRE640480_HSYNC + AMPIRE640480_HBP - 1);
    hLtdcEval.Init.TotalHeigh = (AMPIRE640480_HEIGHT + AMPIRE640480_VSYNC + AMPIRE640480_VBP + AMPIRE640480_VFP - 1);
    hLtdcEval.Init.TotalWidth = (AMPIRE640480_WIDTH + AMPIRE640480_HSYNC + AMPIRE640480_HBP + AMPIRE640480_HFP - 1);

    /* LCD clock configuration */
    BSP_LCD_ClockConfig(&hLtdcEval, NULL);

    /* Initialize the LCD pixel width and pixel height */
    hLtdcEval.LayerCfg->ImageWidth  = AMPIRE640480_WIDTH;
    hLtdcEval.LayerCfg->ImageHeight = AMPIRE640480_HEIGHT;

    /* Background value */
    hLtdcEval.Init.Backcolor.Blue = 0;
    hLtdcEval.Init.Backcolor.Green = 0;
    hLtdcEval.Init.Backcolor.Red = 0;

    /* Polarity */
    hLtdcEval.Init.HSPolarity = LTDC_HSPOLARITY_AL;
    hLtdcEval.Init.VSPolarity = LTDC_VSPOLARITY_AL;
    hLtdcEval.Init.DEPolarity = LTDC_DEPOLARITY_AL;
    hLtdcEval.Init.PCPolarity = LTDC_PCPOLARITY_IPC;
    hLtdcEval.Instance = LTDC;

    if (HAL_LTDC_GetState(&hLtdcEval) == HAL_LTDC_STATE_RESET) {
        /* Initialize the LCD Msp: this __weak function can be rewritten by the application */
        BSP_LCD_MspInit(&hLtdcEval, NULL);
    }
    HAL_LTDC_Init(&hLtdcEval);

    /* set a default layer to the base of sdram */
    BSP_LCD_LayerDefaultInit(0, LCD_FB_START_ADDRESS);
    BSP_LCD_SelectLayer(0);

    /* clear it out */
    memset((void *)hLtdcEval.LayerCfg[ActiveLayer].FBStartAdress, 0, BSP_LCD_GetXSize() * BSP_LCD_GetYSize() * BSP_LCD_PixelSize());

    /* turn the display on */
    BSP_LCD_DisplayOn();

    return LCD_OK;
}

/* LK display api here */
status_t display_get_framebuffer(struct display_framebuffer *fb)
{
    fb->image.pixels = (void *)hLtdcEval.LayerCfg[ActiveLayer].FBStartAdress;

    if (hLtdcEval.LayerCfg[ActiveLayer].PixelFormat == LTDC_PIXEL_FORMAT_ARGB8888) {
        fb->format = DISPLAY_FORMAT_ARGB_8888;
        fb->image.format = IMAGE_FORMAT_ARGB_8888;
        fb->image.rowbytes = BSP_LCD_GetXSize() * 4;
    } else if (hLtdcEval.LayerCfg[ActiveLayer].PixelFormat == LTDC_PIXEL_FORMAT_RGB565) {
        fb->format = DISPLAY_FORMAT_RGB_565;
        fb->image.format = IMAGE_FORMAT_RGB_565;
        fb->image.rowbytes = BSP_LCD_GetXSize() * 2;
    } else {
        panic("unhandled pixel format\n");
        return ERR_NOT_FOUND;
    }

    fb->image.width = BSP_LCD_GetXSize();
    fb->image.height = BSP_LCD_GetYSize();
    fb->image.stride = BSP_LCD_GetXSize();
    fb->flush = NULL;

    return NO_ERROR;
}

status_t display_get_info(struct display_info *info)
{
    if (hLtdcEval.LayerCfg[ActiveLayer].PixelFormat == LTDC_PIXEL_FORMAT_ARGB8888) {
        info->format = DISPLAY_FORMAT_ARGB_8888;
    } else if (hLtdcEval.LayerCfg[ActiveLayer].PixelFormat == LTDC_PIXEL_FORMAT_RGB565) {
        info->format = DISPLAY_FORMAT_RGB_565;
    } else {
        panic("unhandled pixel format\n");
        return ERR_NOT_FOUND;
    }

    info->width = BSP_LCD_GetXSize();
    info->height = BSP_LCD_GetYSize();

    return NO_ERROR;
}

status_t display_present(struct display_image *image, uint starty, uint endy)
{
  TRACEF("display_present - not implemented");
  DEBUG_ASSERT(false);
  return NO_ERROR;
}
