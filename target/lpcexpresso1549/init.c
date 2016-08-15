/*
 * Copyright (c) 2014 Travis Geiselbrecht
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
#include <debug.h>
#include <target.h>
#include <compiler.h>
#include <dev/gpio.h>
#include <platform/gpio.h>

#include <platform/lpc.h>

/* needs to be defined for the lpcopen drivers */
const uint32_t OscRateIn = 12000000;
const uint32_t RTCOscRateIn = 32768;

/* The System initialization code is called prior to the application and
   initializes the board for run-time operation. Board initialization
   includes clock setup and default pin muxing configuration. */

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/* IOCON setup table, only items that need changing from their default pin
   state are in this table. */
STATIC const PINMUX_GRP_T ioconSetup[] = {
    /* LEDs */
    {0, 25,  (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},        /* PIO0_25-BREAK_CTRL-RED (low enable) */
    {0, 3,   (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},        /* PIO0_3-SCT1_OUT4-GRN */
    {1, 1,   (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},        /* PIO1_1-BREAK_STS1-BLUE */

    /* QEI, motor controller, I2C, CAN */
    {0, 2,   (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},        /* PIO0_2-QEI-SCT0_IN */
    {0, 30,  (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},        /* PIO0_30-QEI-SCT0_IN */
    {0, 17,  (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},        /* PIO0_17-QEI-SCT0_IN */
    {0, 25,  (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},        /* PIO0_25-BREAK_CTRL-RED */
    {1, 1,   (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},        /* PIO1_1-BREAK_STS1-BLUE */
    {0, 23,  (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},        /* PIO0_23-I2C_SDA */
    {0, 22,  (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},        /* PIO0_22-I2C_SCL */
    {0, 11,  (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},        /* PIO0_11-CAN_RD */
    {0, 31,  (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},        /* PIO0_31-CAN_TD */

    /* ADC */
    {1, 3,   (IOCON_MODE_INACT)},                           /* PIO1_3-ADC1_5 */
    {0, 4,   (IOCON_MODE_INACT)},                           /* PIO0_4-ADC0_4 */
    {0, 5,   (IOCON_MODE_INACT)},                           /* PIO0_5-ADC0_3 */
    {0, 7,   (IOCON_MODE_INACT)},                           /* PIO0_7-ADC0_1 */
    {0, 8,   (IOCON_MODE_INACT)},                           /* PIO0_8-ADC0_0 */
    {0, 9,   (IOCON_MODE_INACT)},                           /* PIO0_9-ADC1_1 */
    {0, 10,  (IOCON_MODE_INACT)},                           /* PIO0_10-ADC1_2 */

    /* Joystick */
    {1, 4,   (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},        /* PIO1_4-JOY_U */
    {1, 5,   (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},        /* PIO1_5-JOY_C */
    {1, 6,   (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},        /* PIO1_6-JOY_D */
    {1, 7,   (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},        /* PIO1_7-JOY_R */
    {1, 8,   (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},        /* PIO1_8-JOY_L */

    /* UART */
    {0, 13,  (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},        /* PIO0_13-ISP_RX */
    {0, 18,  (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},        /* PIO0_18-ISP_TX */
    {0, 11,  (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},
    {0, 31,  (IOCON_MODE_INACT | IOCON_DIGMODE_EN)},

    /* USB related */
    {1, 11,  (IOCON_MODE_PULLDOWN | IOCON_DIGMODE_EN)}, /* PIO1_11-ISP_1 (VBUS) */
};

/* SWIM pin assignment definitions for pin assignment/muxing */
typedef struct {
    uint16_t assignedpin : 9;       /* Function and mode */
    uint16_t port : 2;              /* Pin port */
    uint16_t pin : 5;               /* Pin number */
} SWM_GRP_T;

/* Pin muxing table, only items that need changing from their default pin
   state are in this table. */
STATIC const SWM_GRP_T swmSetup[] = {
    /* USB related */
    {(uint16_t) SWM_USB_VBUS_I, 1, 11},     /* PIO1_11-ISP_1-AIN_CTRL */

    /* UART */
    {(uint16_t) SWM_UART0_RXD_I, 0, 13},        /* PIO0_13-ISP_RX */
    {(uint16_t) SWM_UART0_TXD_O, 0, 18},        /* PIO0_18-ISP_TX */
};

/* Setup fixed pin functions (GPIOs are fixed) */
/* No fixed pins except GPIOs */
#define PINENABLE0_VAL 0xFFFFFFFF

/* No fixed pins except GPIOs */
#define PINENABLE1_VAL 0x00FFFFFF

/* Sets up system pin muxing */
void Board_SetupMuxing(void)
{
    /* Enable SWM and IOCON clocks */
    Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_IOCON);
    Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_SWM);
    Chip_SYSCTL_PeriphReset(RESET_IOCON);

    /* IOCON setup */
    Chip_IOCON_SetPinMuxing(LPC_IOCON, ioconSetup, sizeof(ioconSetup) / sizeof(PINMUX_GRP_T));

    /* SWM assignable pin setup */
    for (uint i = 0; i < (sizeof(swmSetup) / sizeof(SWM_GRP_T)); i++) {
        Chip_SWM_MovablePortPinAssign((CHIP_SWM_PIN_MOVABLE_T) swmSetup[i].assignedpin,
                                      swmSetup[i].port, swmSetup[i].pin);
    }

    /* SWM fixed pin setup */
    //  LPC_SWM->PINENABLE[0] = PINENABLE0_VAL;
    //  LPC_SWM->PINENABLE[1] = PINENABLE1_VAL;

    /* Note SWM and IOCON clocks are left on */
}

/* Initialize debug output via UART for board */
void Board_Debug_Init(void)
{
    /* Disables pullups/pulldowns and enable digital mode */
    Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 13, (IOCON_MODE_INACT | IOCON_DIGMODE_EN));
    Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 18, (IOCON_MODE_INACT | IOCON_DIGMODE_EN));

    /* UART signal muxing via SWM */
    Chip_SWM_MovablePortPinAssign(SWM_UART0_RXD_I, 0, 13);
    Chip_SWM_MovablePortPinAssign(SWM_UART0_TXD_O, 0, 18);
}

#define MAXLEDS 3
static const uint8_t ledpins[MAXLEDS] = {25, 3, 1};
static const uint8_t ledports[MAXLEDS] = {0, 0, 1};

/* Initializes board LED(s) */
static void Board_LED_Init(void)
{
    int idx;

    Chip_GPIO_Init(LPC_GPIO);

    for (idx = 0; idx < MAXLEDS; idx++) {
        /* Set the GPIO as output with initial state off (high) */
        Chip_GPIO_SetPinDIROutput(LPC_GPIO, ledports[idx], ledpins[idx]);
        Chip_GPIO_SetPinState(LPC_GPIO, ledports[idx], ledpins[idx], true);
    }
}

/* Sets the state of a board LED to on or off */
void Board_LED_Set(uint8_t LEDNumber, bool On)
{
    if (LEDNumber < MAXLEDS) {
        /* Toggle state, low is on, high is off */
        Chip_GPIO_SetPinState(LPC_GPIO, ledports[LEDNumber], ledpins[LEDNumber], !On);
    }
}

/* Returns the current state of a board LED */
bool Board_LED_Test(uint8_t LEDNumber)
{
    bool state = false;

    if (LEDNumber < MAXLEDS) {
        state = !Chip_GPIO_GetPinState(LPC_GPIO, ledports[LEDNumber], ledpins[LEDNumber]);
    }

    return state;
}

/* Toggles the current state of a board LED */
void Board_LED_Toggle(uint8_t LEDNumber)
{
    Chip_GPIO_SetPinToggle(LPC_GPIO, ledports[LEDNumber], ledpins[LEDNumber]);
}


void target_early_init(void)
{
    Board_SetupMuxing();

    Board_Debug_Init();
    Board_LED_Init();
}

void target_init(void)
{
}

void target_set_debug_led(unsigned int led, bool on)
{
    if (led < 3)
        Board_LED_Set(led, on);
}

// vim: set ts=4 sw=4 expandtab:
