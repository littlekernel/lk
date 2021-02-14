// Copyright (c) 2020 Brian Swetland
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#pragma once

#include <hardware/uart.h>

// define how this target will map the debug uart to hardware and pins
#define DEBUG_UART uart0
#define DEBUG_UART_GPIOA 0
#define DEBUG_UART_GPIOB 1
