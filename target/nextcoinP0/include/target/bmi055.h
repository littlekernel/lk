/*
 * Copyright (c) 2015 Eric Holland
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
#ifndef __SENSOR_BMI055_H
#define __SENSOR_BMI055_H

#include <sys/types.h>

#define BMI055_ADDRESS_READ(x) (x | 0x80)
#define BMI055_ADDERSS_WRITE(x) (x & 0x7F)

/**
*       definitions for accelerometer IC
*/
#define BMI055_ACC_BGW_CHPID        0x00
#define BMI055_ACC_ACCD_X_LSB       0X02
#define BMI055_ACC_ACCD_X_MSB       0X03
#define BMI055_ACC_ACCD_Y_LSB       0X04
#define BMI055_ACC_ACCD_Y_MSB       0X05
#define BMI055_ACC_ACCD_Z_LSB       0X06
#define BMI055_ACC_ACCD_Z_MSB       0X07
#define BMI055_ACC_ACCD_TEMP        0X08
#define BMI055_ACC_INT_STATUS_0     0X09
#define BMI055_ACC_INT_STATUS_1     0X0a
#define BMI055_ACC_INT_STATUS_2     0x0b
#define BMI055_ACC_INT_STATUS_3     0x0c
#define BMI055_ACC_FIFO_STATUS      0x0e
#define BMI055_ACC_PMU_RANGE        0x0f
#define BMI055_ACC_PMU_BW           0x10
#define BMI055_ACC_PMU_LPW          0x11
#define BMI055_ACC_PMU_LOW_POWER    0x12
#define BMI055_ACC_ACCD_HBW         0x13
#define BMI055_ACC_BGW_SOFTRESET    0x14
#define BMI055_ACC_INT_EN_0         0x16
#define BMI055_ACC_INT_EN_1         0x17
#define BMI055_ACC_INT_EN_2         0x18
#define BMI055_ACC_INT_MAP_0        0x19
#define BMI055_ACC_INT_MAP_1        0x1A
#define BMI055_ACC_INT_MAP_2        0x1B
#define BMI055_ACC_INT_SRC          0x1E
#define BMI055_ACC_INT_OUT_CTRL     0x20
#define BMI055_ACC_INT_RST_LATCH    0x21
#define BMI055_ACC_INT_0            0x22
#define BMI055_ACC_INT_1            0x23
#define BMI055_ACC_INT_2            0x24
#define BMI055_ACC_INT_3            0x25
#define BMI055_ACC_INT_4            0x26
#define BMI055_ACC_INT_5            0x27
#define BMI055_ACC_INT_6            0x28
#define BMI055_ACC_INT_7            0x29
#define BMI055_ACC_INT_8            0x2a
#define BMI055_ACC_INT_9            0x2b
#define BMI055_ACC_INT_A            0x2c
#define BMI055_ACC_INT_B            0x2d
#define BMI055_ACC_INT_C            0x2e
#define BMI055_ACC_INT_D            0x2f
#define BMI055_ACC_FIFO_CONFIG_0    0x30
#define BMI055_ACC_PMU_SELF_TEST    0x32
#define BMI055_ACC_TRIM_NVM_CTRL    0x33
#define BMI055_ACC_BGW_SPI3_WDT     0x34
#define BMI055_ACC_OFC_CTRL         0x36
#define BMI055_ACC_OFC_SETTING      0x37
#define BMI055_ACC_OFC_OFFSET_X     0x38
#define BMI055_ACC_OFC_OFFSET_Y     0x39
#define BMI055_ACC_OFC_OFFSET_Z     0x3A
#define BMI055_ACC_TRIM_GP0         0x3B
#define BMI055_ACC_TRIM_GP1         0x3C
#define BMI055_ACC_FIFO_CONFIG_1    0x3E
#define BMI055_ACC_FIFO_DATA        0x3F


/**
*       definitions for gyro IC
*/
#define BMI055_GYRO_CHIP_ID          0x00
#define BMI055_GYRO_RATE_X_LSB       0X02
#define BMI055_GYRO_RATE_X_MSB       0X03
#define BMI055_GYRO_RATE_Y_LSB       0X04
#define BMI055_GYRO_RATE_Y_MSB       0X05
#define BMI055_GYRO_RATE_Z_LSB       0X06
#define BMI055_GYRO_RATE_Z_MSB       0X07
#define BMI055_GYRO_INT_STATUS_0     0X09
#define BMI055_GYRO_INT_STATUS_1     0X0a
#define BMI055_GYRO_INT_STATUS_2     0x0b
#define BMI055_GYRO_INT_STATUS_3     0x0c
#define BMI055_GYRO_FIFO_STATUS      0x0e
#define BMI055_GYRO_RANGE            0x0f
#define BMI055_GYRO_BW               0x10
#define BMI055_GYRO_LPM1             0x11
#define BMI055_GYRO_LPM2             0x12
#define BMI055_GYRO_RATE_HBW         0x13
#define BMI055_GYRO_BGW_SOFTRESET    0x14
#define BMI055_GYRO_INT_EN_0         0x15
#define BMI055_GYRO_INT_EN_1         0x16
#define BMI055_GYRO_INT_MAP_0        0x17
#define BMI055_GYRO_INT_MAP_1        0x18
#define BMI055_GYRO_INT_MAP_2        0x19
#define BMI055_GYRO_0X1A             0x1A
#define BMI055_GYRO_0X1B             0x1B
#define BMI055_GYRO_0X1C             0x1C
#define BMI055_GYRO_0X1E             0x1E
#define BMI055_GYRO_INT_RST_LATCH    0x21
#define BMI055_GYRO_HIGH_TH_X        0x22
#define BMI055_GYRO_HIGH_DUR_X       0x23
#define BMI055_GYRO_HIGH_TH_Y        0x24
#define BMI055_GYRO_HIGH_DUR_Y       0x25
#define BMI055_GYRO_HIGH_TH_Z        0x26
#define BMI055_GYRO_HIGH_DUR_Z       0x27
#define BMI055_GYRO_SOC              0x31
#define BMI055_GYRO_A_FOC            0x32
#define BMI055_GYRO_TRIM_NVM_CTRL    0x33
#define BMI055_GYRO_BGW_SPI3_WDT     0x34
#define BMI055_GYRO_OFC1             0x36
#define BMI055_GYRO_OFC2             0x37
#define BMI055_GYRO_OFC3             0x38
#define BMI055_GYRO_OFC4             0x39
#define BMI055_GYRO_TRIM_GP0         0x3A
#define BMI055_GYRO_TRIM_GP1         0x3B
#define BMI055_GYRO_BIST             0x3C
#define BMI055_GYRO_FIFO_CONFIG_0    0x3D
#define BMI055_GYRO_FIFO_CONFIG_1    0x3E








#endif

