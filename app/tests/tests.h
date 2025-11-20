/*
 * Copyright (c) 2008-2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/console_cmd.h>

int benchmarks(int argc, const console_cmd_args *argv);
int clock_tests(int argc, const console_cmd_args *argv);
int fibo(int argc, const console_cmd_args *argv);
int mem_test(int argc, const console_cmd_args *argv);
int port_tests(int argc, const console_cmd_args *argv);
int thread_tests(int argc, const console_cmd_args *argv);
