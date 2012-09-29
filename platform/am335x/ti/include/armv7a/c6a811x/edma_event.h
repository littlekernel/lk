/**
 *  \file   edma_event.h
 *
 *  \brief  EDMA event enumeration
 */

/* Copyright (C) 2012 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED */

#ifndef _EDMAEVENT_H
#define _EDMAEVENT_H

#include "hw_types.h"

#ifdef __cplusplus
extern "C" {
#endif
/****************************************************************************
**                       MACRO DEFINITIONS
****************************************************************************/

/********************* Direct Mapped Events ********************************/

    /* McASP0 Events */
    #define EDMA3_CHA_McASP0_TX             8
    #define EDMA3_CHA_McASP0_RX             9
    /* McASP1 Events */
    #define EDMA3_CHA_McASP1_TX             10
    #define EDMA3_CHA_McASP1_RX             11
    /* McASP2 Events */
    #define EDMA3_CHA_McASP2_TX             12
    #define EDMA3_CHA_McASP2_RX             13
    /* McASP3 Events */
    #define EDMA3_CHA_McASP3_TX             56
    #define EDMA3_CHA_McASP3_RX             57
    /* McASP4 Events */
    #define EDMA3_CHA_McASP4_TX             62
    #define EDMA3_CHA_McASP4_RX             63

    /* McBSP Events */
    #define EDMA3_CHA_McBSP_TX              14
    #define EDMA3_CHA_McBSP_RX              15

    /* PCIe Events */
    #define EDMA3_CHA_PCIe_TX               54
    #define EDMA3_CHA_PCIe_RX               55

    /* SD0 Events */
    #define EDMA3_CHA_SD0_TX                24
    #define EDMA3_CHA_SD0_RX                25
    /* SD1 Events */
    #define EDMA3_CHA_SD1_TX                2
    #define EDMA3_CHA_SD1_RX                3
    
    /* UART0 Events */
    #define EDMA3_CHA_UART0_TX              26
    #define EDMA3_CHA_UART0_RX              27
    /* UART1 Events */
    #define EDMA3_CHA_UART1_TX              28
    #define EDMA3_CHA_UART1_RX              29
    /* UART2 Events */
    #define EDMA3_CHA_UART2_TX              30
    #define EDMA3_CHA_UART2_RX              31

    /* TIMER Events */
    #define EDMA3_CHA_TIMER4                48
    #define EDMA3_CHA_TIMER5                49
    #define EDMA3_CHA_TIMER6                50
    #define EDMA3_CHA_TIMER7                51

    /* SPI0 Events */
    #define EDMA3_CHA_SPI0_CH0_TX           16
    #define EDMA3_CHA_SPI0_CH0_RX           17
    #define EDMA3_CHA_SPI0_CH1_TX           18
    #define EDMA3_CHA_SPI0_CH1_RX           19
    #define EDMA3_CHA_SPI0_CH2_TX           20
    #define EDMA3_CHA_SPI0_CH2_RX           21
    #define EDMA3_CHA_SPI0_CH3_TX           22
    #define EDMA3_CHA_SPI0_CH3_RX           23
    /* SPI1 Events */
    #define EDMA3_CHA_SPI1_CH0_TX           42
    #define EDMA3_CHA_SPI1_CH0_RX           43
    #define EDMA3_CHA_SPI1_CH1_TX           44
    #define EDMA3_CHA_SPI1_CH1_RX           45

    /* I2C0 Events */
    #define EDMA3_CHA_I2C0_TX               58
    #define EDMA3_CHA_I2C0_RX               59
    /* I2C1 Events */
    #define EDMA3_CHA_I2C1_TX               60
    #define EDMA3_CHA_I2C1_RX               61

    /* GPMC Events */
    #define EDMA3_CHA_GPMC                  52

    /* DCAN0 Events */
    #define EDMA3_CHA_DCAN0_IF1             40
    #define EDMA3_CHA_DCAN0_IF2             41
    #define EDMA3_CHA_DCAN0_IF3             47

/********************** Cross Mapped Events ********************************/

    /* TBD */

/********************** Event Mux Values ***********************************/

    /* Default Event */
    #define EDMA3_MUX_Default               0

    /* McASP5 Events */
    #define EDMA3_MUX_McASP5_TX             26
    #define EDMA3_MUX_McASP5_RX             27

    /* GPIO4 Events */
    #define EDMA3_MUX_GPIO4                 49
    /* GPIO5 Events */
    #define EDMA3_MUX_GPIO5                 50

    /* ADCFIFO Events */
    #define EDMA3_MUX_ADCFIFO               32
    #define EDMA3_MUX_ADCFIFO1              33

    /* eHRPWM_EVT0 Events */
    #define EDMA3_MUX_eHRPWM_EVT0           37
    /* eHRPWM_EVT1 Events */
    #define EDMA3_MUX_eHRPWM_EVT1           38
    /* eHRPWM_EVT2 Events */
    #define EDMA3_MUX_eHRPWM_EVT2           39

    /* eQEP_EVT0 Events */
    #define EDMA3_MUX_eQEP_EVT0             40
    /* eQEP_EVT1 Events */
    #define EDMA3_MUX_eQEP_EVT1             41
    /* eQEP_EVT2 Events */
    #define EDMA3_MUX_eQEP_EVT2             42

    /* PRUSSv2 Events */
    #define EDMA3_MUX_PRU1HST7              43
    #define EDMA3_MUX_PRU1HST6              44

    /* SD2 Events */
    #define EDMA3_MUX_SD2_TX                1
    #define EDMA3_MUX_SD2_RX                2

    /* TIMER1 Events */
    #define EDMA3_MUX_TIMER1                23
    /* TIMER2 Events */
    #define EDMA3_MUX_TIMER2                24
    /* TIMER3 Events */
    #define EDMA3_MUX_TIMER3                25

    /* UART3 Events */
    #define EDMA3_MUX_UART3_TX              7
    #define EDMA3_MUX_UART3_RX              8
    /* UART4 Events */
    #define EDMA3_MUX_UART4_TX              9
    #define EDMA3_MUX_UART4_RX              10
    /* UART5 Events */
    #define EDMA3_MUX_UART5_TX              11
    #define EDMA3_MUX_UART5_RX              12
    /* UART6 Events */
    #define EDMA3_MUX_UART6_TX              45
    #define EDMA3_MUX_UART6_RX              46
    /* UART7 Events */
    #define EDMA3_MUX_UART7_TX              47
    #define EDMA3_MUX_UART7_RX              48

    /* External EDMA Events */
    #define EDMA3_MUX_EDMA_EVT0             28
    #define EDMA3_MUX_EDMA_EVT1             29
    #define EDMA3_MUX_EDMA_EVT2             30
    #define EDMA3_MUX_EDMA_EVT3             31

    /* SPI2 Events */
    #define EDMA3_MUX_SPI2_CH0_TX           16
    #define EDMA3_MUX_SPI2_CH0_RX           17
    #define EDMA3_MUX_SPI2_CH1_TX           18
    #define EDMA3_MUX_SPI2_CH1_RX           19
    /* SPI3 Events */
    #define EDMA3_MUX_SPI3_CH0_TX           20
    #define EDMA3_MUX_SPI3_CH0_RX           21

    /* I2C2 Events */
    #define EDMA3_MUX_I2C2_TX               3
    #define EDMA3_MUX_I2C2_RX               4
    /* I2C3 Events */
    #define EDMA3_MUX_I2C3_TX               5
    #define EDMA3_MUX_I2C3_RX               6

    /* DCAN1 Events */
    #define EDMA3_MUX_DCAN1_IF1             13
    #define EDMA3_MUX_DCAN1_IF2             14
    #define EDMA3_MUX_DCAN1_IF3             15

    /* eCAP_EVT0 Events */
    #define EDMA3_MUX_eCAP_EVT0             34
    /* eCAP_EVT1 Events */
    #define EDMA3_MUX_eCAP_EVT1             35
    /* eCAP_EVT2 Events */
    #define EDMA3_MUX_eCAP_EVT2             36


#ifdef __cplusplus
}
#endif

#endif
