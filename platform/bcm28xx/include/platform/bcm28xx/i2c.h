#pragma once

#include <platform/bcm28xx.h>

#define I2C0_BASE               (BCM_PERIPH_BASE_VIRT + 0x205000)
#define I2C1_BASE               (BCM_PERIPH_BASE_VIRT + 0x804000)
#define I2C2_BASE               (BCM_PERIPH_BASE_VIRT + 0x805000)
#define I2C3_BASE               (BCM_PERIPH_BASE_VIRT + 0x205600)
#define I2C4_BASE               (BCM_PERIPH_BASE_VIRT + 0x205800)
#define I2C5_BASE               (BCM_PERIPH_BASE_VIRT + 0x205a00)
#define I2C6_BASE               (BCM_PERIPH_BASE_VIRT + 0x205c00)
#define I2C7_BASE               (BCM_PERIPH_BASE_VIRT + 0x205e00)

#define I2C_C_READ              0x00000001
#define I2C_C_CLEAR0            0x00000010
#define I2C_C_CLEAR1            0x00000020
#define I2C_C_ST                0x00000080
#define I2C_C_I2CEN             0x00008000

#define I2C_S_TA                0x00000001
#define I2C_S_DONE              0x00000002
#define I2C_S_TXD               0x00000010
#define I2C_S_RXD               0x00000020
#define I2C_S_ERR               0x00000100
#define I2C_S_CLKT              0x00000200

#ifdef __cplusplus
extern "C" {
#endif
void i2c_set_rate(unsigned busnum, unsigned long rate);
int i2c_xfer(unsigned busnum, unsigned addr,
	     char *sendbuf, size_t sendsz,
	     char *recvbuf, size_t recvsz);
#ifdef __cplusplus
}
#endif
