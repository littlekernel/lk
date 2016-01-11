/* swd.h 
 *
 * Copyright 2011 Brian Swetland <swetland@frotz.net>
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _SWDP_H_
#define _SWDP_H_

void swd_init(void);
void swd_reset(void);
int swd_write(unsigned reg, unsigned val);
int swd_read(unsigned reg, unsigned *val);

unsigned swd_set_clock(unsigned khz);
unsigned swo_set_clock(unsigned khz);
void swd_hw_reset(int assert);

void jtag_init(void);
int jtag_io(unsigned count, unsigned tms, unsigned tdi, unsigned *tdo);

// swdp_read/write() register codes

// Park Stop Parity Addr3 Addr2 RnW APnDP Start

#define RD_IDCODE	0b10100101
#define RD_DPCTRL	0b10001101
#define RD_RESEND	0b10010101
#define RD_BUFFER	0b10111101

#define WR_ABORT	0b10000001
#define WR_DPCTRL	0b10101001
#define WR_SELECT	0b10110001
#define WR_BUFFER	0b10011001

#define RD_AP0		0b10000111
#define RD_AP1		0b10101111
#define RD_AP2		0b10110111
#define RD_AP3		0b10011111

#define WR_AP0		0b10100011
#define WR_AP1		0b10001011
#define WR_AP2		0b10010011
#define WR_AP3		0b10111011

#endif

