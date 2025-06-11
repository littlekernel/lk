/*
 * Copyright (c) 2008-2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#ifndef __APP_TESTS_H
#define __APP_TESTS_H

#include <lk/console_cmd.h>

int cbuf_tests(int argc, const console_cmd_args *argv);
int fibo(int argc, const console_cmd_args *argv);
int port_tests(int argc, const console_cmd_args *argv);
int spinner(int argc, const console_cmd_args *argv);
int thread_tests(int argc, const console_cmd_args *argv);
int benchmarks(int argc, const console_cmd_args *argv);
int clock_tests(int argc, const console_cmd_args *argv);
int v9p_tests(int argc, const console_cmd_args *argv);
int v9fs_tests(int argc, const console_cmd_args *argv);

#endif

