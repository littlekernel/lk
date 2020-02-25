#pragma once

uint32_t otp_read(uint8_t addr);
void otp_pretty_print(void);

#define OTP_ERR_PROGRAM -1
#define OTP_ERR_ENABLE  -2
#define OTP_ERR_DISABLE -3

int otp_write(uint8_t addr, uint32_t val);
