/**
 *  \file   dmtimer.h
 *
 *  \brief  DMTimer API prototypes and macros.
 *
 *   This file contains the driver API prototypes and macro definitions.
 */

/*
* Copyright (C) 2010 Texas Instruments Incorporated - http://www.ti.com/
*/
/*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*    Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*
*    Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the
*    distribution.
*
*    Neither the name of Texas Instruments Incorporated nor the names of
*    its contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/

#ifndef _DMTIMER_H_
#define _DMTIMER_H_

#include "hw_dmtimer.h"
#include "hw_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************/
/*
** Values that can be passed to DMTimerPreScalerClkEnable as ptv so as to derive
** Pre-Scalar clock from timer clock.
*/
/* Value used to divide timer clock by 2 */
#define DMTIMER_PRESCALER_CLK_DIV_BY_2      ((0 << DMTIMER_TCLR_PTV_SHIFT) | \
                                              DMTIMER_TCLR_PRE)

/* Value used to divide timer clock by 4 */
#define DMTIMER_PRESCALER_CLK_DIV_BY_4	    ((1 << DMTIMER_TCLR_PTV_SHIFT) | \
                                              DMTIMER_TCLR_PRE)

/* Value used to divide timer clock by 8 */
#define DMTIMER_PRESCALER_CLK_DIV_BY_8	    ((2 << DMTIMER_TCLR_PTV_SHIFT) | \
                                              DMTIMER_TCLR_PRE)

/* Value used to divide timer clock by 16 */
#define DMTIMER_PRESCALER_CLK_DIV_BY_16	    ((3 << DMTIMER_TCLR_PTV_SHIFT) | \
                                              DMTIMER_TCLR_PRE)

/* Value used to divide timer clock by 32 */
#define DMTIMER_PRESCALER_CLK_DIV_BY_32	    ((4 << DMTIMER_TCLR_PTV_SHIFT) | \
                                              DMTIMER_TCLR_PRE)

/* Value used to divide timer clock by 64 */
#define DMTIMER_PRESCALER_CLK_DIV_BY_64	    ((5 << DMTIMER_TCLR_PTV_SHIFT) | \
                                              DMTIMER_TCLR_PRE)

/* Value used to divide timer clock by 128 */
#define DMTIMER_PRESCALER_CLK_DIV_BY_128    ((6 << DMTIMER_TCLR_PTV_SHIFT) | \
                                              DMTIMER_TCLR_PRE)

/* Value used to divide timer clock by 256 */
#define DMTIMER_PRESCALER_CLK_DIV_BY_256    ((7 << DMTIMER_TCLR_PTV_SHIFT) | \
                                              DMTIMER_TCLR_PRE)

/******************************************************************************/
/*
** Values that can be passed to DMTimerModeConfigure as timerMode.
*/
/* Value used to enable the timer in one-shot and compare mode */
#define DMTIMER_ONESHOT_CMP_ENABLE          (DMTIMER_TCLR_CE)

/* Value used to enable the timer only in one-shot mode */
#define DMTIMER_ONESHOT_NOCMP_ENABLE        (0x0000000)

/* Value used to enable the timer in auto-reload and compare mode */
#define DMTIMER_AUTORLD_CMP_ENABLE          (DMTIMER_TCLR_AR | DMTIMER_TCLR_CE)

/* Value used to enable the timer only in auto-reload mode */
#define DMTIMER_AUTORLD_NOCMP_ENABLE        (DMTIMER_TCLR_AR)

/******************************************************************************/
/*
** Values that can be passed to DMTimerPWMEnable as ptMode. 
*/
/* Value used to determine Pulse(Positive) mode for PORTTIMERPWM */
#define DMTIMER_PWM_PULSE_MODE_POSITIVE	    (DMTIMER_TCLR_PT_PULSE | \
                                            (~DMTIMER_TCLR_SCPWM))

/* Value used to determine Pulse(Negative) mode for PORTTIMERPWM */
#define DMTIMER_PWM_PULSE_MODE_NEGATIVE	    (DMTIMER_TCLR_PT_PULSE | \
                                            (DMTIMER_TCLR_SCPWM))

/* Value used to determine Toggle(Positive) mode for PORTTIMERPWM */
#define DMTIMER_PWM_TOGGLE_MODE_POSITIVE    (DMTIMER_TCLR_PT | \
                                            (~DMTIMER_TCLR_SCPWM))

/* Value used to determine Toggle(Negative) mode for PORTTIMERPWM */
#define DMTIMER_PWM_TOGGLE_MODE_NEGATIVE    (DMTIMER_TCLR_PT | \
                                            (DMTIMER_TCLR_SCPWM))

/******************************************************************************/
/*
** Values that can be passed to DMTimerPWMEnable as trgOpMode.
*/
/* Enable trigger on PORTIMERPWM for overflow */
#define DMTIMER_PWM_TRIGON_OVRFLW	    (DMTIMER_TCLR_TRG_OVERFLOW << \
					     DMTIMER_TCLR_TRG_SHIFT)
/* Enable trigger on PORTIMERPWM for overflow and match */
#define DMTIMER_PWM_TRIGON_OVRFLW_MATCH    (DMTIMER_TCLR_TRG_OVERFLOWANDMATCH << \
					     DMTIMER_TCLR_TRG_SHIFT)

/******************************************************************************/
/*
** Values that can be passed to DMTimerCaptureConfigure as tcm.
*/
/* Capture disabled */
#define DMTIMER_CAPTURE_NO		    (DMTIMER_TCLR_TCM_NOCAPTURE)

/* Capture on low-high transition signal on PIEVENTCAPT */
#define DMTIMER_CAPTURE_LOWTOHIGH           (DMTIMER_TCLR_TCM_LOWHIGH << \
					     DMTIMER_TCLR_TCM_SHIFT)

/* Capture on high-low transition signal on PIEVENTCAPT */
#define DMTIMER_CAPTURE_HIGHTOLOW           (DMTIMER_TCLR_TCM_HIGHLOW << \
					     DMTIMER_TCLR_TCM_SHIFT)

/* Capture on low as well as high transition signal on PIEVENTCAPT */
#define DMTIMER_CAPTURE_BOTH                (DMTIMER_TCLR_TCM_BOTH << \
					     DMTIMER_TCLR_TCM_SHIFT)

/******************************************************************************/
/*
** Values that can be passed to DMTimerCaptureConfigure as captMode.
*/
/* Value used to enable capture of single event on PIEVENTCAPT */
#define DMTIMER_CAPTURE_SINGLE		    (DMTIMER_TCLR_CAPT_MODE_SINGLE)

/* Value used to enable capture of second event on PIEVENTCAPT */
#define DMTIMER_CAPTURE_SECOND		    (DMTIMER_TCLR_CAPT_MODE)

/******************************************************************************/
/*
** Values that can be passed to DMTimerGPOConfigure as gpoCfg.
*/
/* Value used to drive 0 on PORGPOCFG pin */
#define DMTIMER_GPO_CFG_0		    (DMTIMER_TCLR_GPO_CFG_DRIVE0)

/* Value used to drive 1 on PORGPOCFG pin */
#define DMTIMER_GPO_CFG_1		    (DMTIMER_TCLR_GPO_CFG_DRIVE1 << \
					     DMTIMER_TCLR_GPO_CFG_SHIFT)

/******************************************************************************/
/*
** Values that can be passed to DMTimerEmulationModeConfigure as emuMode. 
*/
/* Value used to signify that the Emulation mode is free */
#define DMTIMER_EMUFREE_FREE		    (DMTIMER_TIOCP_CFG_EMUFREE)

/* Value used to signify that the Emulation mode is frozen */
#define DMTIMER_EMUFREE_FROZEN              (~DMTIMER_TIOCP_CFG_EMUFREE)

/******************************************************************************/
/*
** Values that can be passed to DMTimerPwrMngmntConfigure as pwrMngMode. 
*/
/* Value used to configure force-idle mode */
#define DMTIMER_FORCE_IDLE_MODE		    (DMTIMER_TIOCP_CFG_IDLEMODE_FORCE)

/* Value used to configure no-idle mode */
#define DMTIMER_NO_IDLE_MODE		    (DMTIMER_TIOCP_CFG_IDLEMODE_NOIDLE << \
					     DMTIMER_TIOCP_CFG_IDLEMODE_SHIFT)

/* Value used to configure smart-idle mode */
#define DMTIMER_SMART_IDLE_MODE		    (DMTIMER_TIOCP_CFG_IDLEMODE_SMART << \
					     DMTIMER_TIOCP_CFG_IDLEMODE_SHIFT)

/* Value used to configure smart-wakeup mode */
#define DMTIMER_SMART_WKUP_MODE		    (DMTIMER_TIOCP_CFG_IDLEMODE_WAKEUP << \
					     DMTIMER_TIOCP_CFG_IDLEMODE_SHIFT)

/******************************************************************************/
/*
** Values that can be passed to DMTimerIntStatusClear/DMTimerIntRawStatusSet/
** as intFlags. Also these values can be used while checking the status got from 
** DMTimerIntRawStatusGet/DMTimerIntStatusGet.
** Any combination is also followed.
** Example- (DMTIMER_INT_TCAR_IT_FLAG | DMTIMER_INT_OVF_IT_FLAG)
*/
/* Value used for capture event of DMTimer */
#define DMTIMER_INT_TCAR_IT_FLAG             (DMTIMER_IRQSTATUS_RAW_TCAR_IT_FLAG)

/* Value used for overflow event of DMTimer */
#define DMTIMER_INT_OVF_IT_FLAG              (DMTIMER_IRQSTATUS_RAW_OVF_IT_FLAG) 

/* Value used for Match event of DMTimer */
#define DMTIMER_INT_MAT_IT_FLAG              (DMTIMER_IRQSTATUS_RAW_MAT_IT_FLAG)

/******************************************************************************/
/*
** Values that can be passed to DMTimerIntEnable/DMTimerIntDisable as intFlags.
** Also these values can be used while checking the status got from 
** DMTimerIntEnableGet.
** Any combination is also followed.
** Example- (DMTIMER_INT_TCAR_EN_FLAG | DMTIMER_INT_OVF_EN_FLAG)
*/
/* Value used for capture event of DMTimer */
#define DMTIMER_INT_TCAR_EN_FLAG             (DMTIMER_IRQENABLE_SET_TCAR_EN_FLAG)

/* Value used for overflow event of DMTimer */
#define DMTIMER_INT_OVF_EN_FLAG              (DMTIMER_IRQENABLE_SET_OVF_EN_FLAG)

/* Value used for Match event of DMTimer */
#define DMTIMER_INT_MAT_EN_FLAG              (DMTIMER_IRQENABLE_SET_MAT_EN_FLAG)

/******************************************************************************/
/*
** Values that can be passed to DMTimerWakeUpEnable and DMTimerWakeUpDisable as
** wakeUpFlags.
** Any combination is also followed.
** Example- (DMTIMER_COMPARE_WKUP | DMTIMER_OVRFLW_WKUP)
*/
/* Value used for wakeup generation for match */
#define DMTIMER_MATCH_WKUP                   (DMTIMER_IRQWAKEEN_MAT_WUP_ENA)

/* Value used for wakeup generation for overflow */
#define DMTIMER_OVRFLW_WKUP                  (DMTIMER_IRQWAKEEN_OVF_WUP_ENA)

/* Value used for wakeup generation for compare */
#define DMTIMER_COMPARE_WKUP                 (DMTIMER_IRQWAKEEN_TCAR_WUP_ENA)

/******************************************************************************/
/*
** Values that can be passed to DMTimerPostedModeConfigure as posted.
*/
/* Value used enable the posted mode of operation */
#define DMTIMER_POSTED_ACTIVE                (DMTIMER_TSICR_POSTED)

/* Value used to disable the posted mode of operation */
#define DMTIMER_POSTED_INACTIVE              (DMTIMER_TSICR_POSTED_INACTIVE)

/******************************************************************************/
/*
** Values that can be used while checking status received from 
** DMTimerWritePostedStatusGet.
*/
/* Value used to check the write posted condition for TMAR register */
#define DMTIMER_WRITE_POST_TMAR              (DMTIMER_TWPS_W_PEND_TMAR)

/* Value used to check the write posted condition for TTGR register */
#define DMTIMER_WRITE_POST_TTGR              (DMTIMER_TWPS_W_PEND_TTGR)

/* Value used to check the write posted condition for TLDR register */
#define DMTIMER_WRITE_POST_TLDR              (DMTIMER_TWPS_W_PEND_TLDR) 

/* Value used to check the write posted condition for TCRR register */
#define DMTIMER_WRITE_POST_TCRR              (DMTIMER_TWPS_W_PEND_TCRR)

/* Value used to check the write posted condition for TCLR register */
#define DMTIMER_WRITE_POST_TCLR              (DMTIMER_TWPS_W_PEND_TCLR)

/******************************************************************************/
/*
** Values that can be passed to DMTimerResetConfigure as rstOption.
*/
/* Value used to enable software reset for DMTimer */
#define DMTIMER_SFT_RESET_ENABLE             (DMTIMER_TSICR_SFT_RESETENABLE)

/* Value used to disable software reset for DMTimer */
#define DMTIMER_SFT_RESET_DISABLE            (DMTIMER_TSICR_SFT)

/******************************************************************************/
/*
** Values that can be used while checking the status received from 
** DMTimerIsResetDone.
*/
/* Value used to check whether reset is done */
#define DMTIMER_IS_RESET_DONE                (DMTIMER_TIOCP_CFG_SOFTRESET_DONE)

/* Value used to check whether reset is ongoing */
#define DMTIMER_IS_RESET_ONGOING             (DMTIMER_TIOCP_CFG_SOFTRESET_ONGOING)

/******************************************************************************/
/*
** Structure to store the DM timer context
*/
typedef struct dmtimerContext{
    unsigned int tldr;
    unsigned int tmar;
    unsigned int irqenableset;
    unsigned int tcrr;
    unsigned int tclr;
}DMTIMERCONTEXT;

/*
** Prototype of the APIs
*/
extern void DMTimerEnable(unsigned int baseAdd);
extern void DMTimerDisable(unsigned int baseAdd);
extern void DMTimerModeConfigure(unsigned int baseAdd, unsigned int timerMode);
extern void DMTimerPreScalerClkEnable(unsigned int baseAdd, unsigned int ptv);
extern void DMTimerPreScalerClkDisable(unsigned int baseAdd);
extern void DMTimerCounterSet(unsigned int baseAdd, unsigned int counter);
extern unsigned int DMTimerCounterGet(unsigned int baseAdd);
extern void DMTimerReloadSet(unsigned int baseAdd, unsigned int reload);
extern unsigned int DMTimerReloadGet(unsigned int baseAdd);
extern void DMTimerPWMEnable(unsigned int baseAdd, unsigned int ptMode,
                      unsigned int trgOpMode);
extern void DMTimerPWMDisable(unsigned int baseAdd);
extern void DMTimerCaptureConfigure(unsigned int baseAdd, unsigned int tcm,
                             unsigned int captMode);
extern void DMTimerGPOConfigure(unsigned int baseAdd, unsigned int gpoCfg);
extern void DMTimerCompareSet(unsigned int baseAdd, unsigned int compareVal);
extern unsigned int DMTimerCompareGet(unsigned int baseAdd);
extern unsigned int DMTimerCapture1Get(unsigned int baseAdd);
extern unsigned int DMTimerCapture2Get(unsigned int baseAdd);
extern void DMTimerEmulationModeConfigure(unsigned int baseAdd, unsigned int emuMode);
extern void DMTimerPwrMngmntConfigure(unsigned int baseAdd, unsigned int pwrMngMode);
extern void DMTimerEndOfInterrupt(unsigned int baseAdd);
extern void DMTimerIntRawStatusSet(unsigned int baseAdd, unsigned int intFlags);
extern unsigned int DMTimerIntRawStatusGet(unsigned int baseAdd);
extern unsigned int DMTimerIntStatusGet(unsigned int baseAdd);
extern void DMTimerIntStatusClear(unsigned int baseAdd, unsigned int intFlags);
extern void DMTimerIntEnable(unsigned int baseAdd, unsigned int intFlags);
extern void DMTimerIntDisable(unsigned int baseAdd, unsigned int intFlags);
extern void DMTimerTriggerSet(unsigned int baseAdd);
extern void DMTimerPostedModeConfigure(unsigned int baseAdd, unsigned int posted);
extern unsigned int DMTimerIntEnableGet(unsigned int baseAdd);
extern void DMTimerWakeUpEnable(unsigned int baseAdd, unsigned int wakeUpFlags);
extern void DMTimerWakeUpDisable(unsigned int baseAdd, unsigned int wakeUpFlags);
extern unsigned int DMTimerWritePostedStatusGet(unsigned int baseAdd);
extern void DMTimerResetConfigure(unsigned int baseAdd, unsigned int rstOption);
extern void DMTimerReset(unsigned int baseAdd);
extern void DMTimerContextSave(unsigned int baseAdd, DMTIMERCONTEXT *contextPtr);
extern void DMTimerContextRestore(unsigned int baseAdd, DMTIMERCONTEXT *contextPtr);

#ifdef __cplusplus
}
#endif

#endif

/******************************** END OF FILE *********************************/




