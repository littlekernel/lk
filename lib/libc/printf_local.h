/*
 * Copyright (c) 2025 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#pragma once

#include <sys/types.h>

#define LONGFLAG       0x00000001
#define LONGLONGFLAG   0x00000002
#define HALFFLAG       0x00000004
#define HALFHALFFLAG   0x00000008
#define SIZETFLAG      0x00000010
#define INTMAXFLAG     0x00000020
#define PTRDIFFFLAG    0x00000040
#define ALTFLAG        0x00000080
#define CAPSFLAG       0x00000100
#define SHOWSIGNFLAG   0x00000200
#define SIGNEDFLAG     0x00000400
#define LEFTFORMATFLAG 0x00000800
#define LEADZEROFLAG   0x00001000
#define BLANKPOSFLAG   0x00002000
#define HAS_PRECISION  0x00004000

extern char *_printf_longlong_to_string(char *buf, unsigned long long n, size_t len, uint flag, char *signchar);
extern char *_printf_longlong_to_hexstring(char *buf, unsigned long long u, size_t len, uint flag);
extern char *_printf_double_to_string(char *buf, size_t len, uint64_t float_bits, uint flag);
extern char *_printf_double_to_hexstring(char *buf, size_t len, uint64_t float_bits, uint flag);
