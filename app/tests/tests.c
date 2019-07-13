/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <app.h>
#include <lk/debug.h>
#include <app/tests.h>
#include <lk/compiler.h>

#include <lk/console_cmd.h>

STATIC_COMMAND_START
STATIC_COMMAND("printf_tests", "test printf", &printf_tests)
STATIC_COMMAND("printf_tests_float", "test printf with floating point", &printf_tests_float)
STATIC_COMMAND("thread_tests", "test the scheduler", &thread_tests)
STATIC_COMMAND("port_tests", "test the ports", &port_tests)
STATIC_COMMAND("clock_tests", "test clocks", &clock_tests)
STATIC_COMMAND("bench", "miscellaneous benchmarks", &benchmarks)
STATIC_COMMAND("fibo", "threaded fibonacci", &fibo)
STATIC_COMMAND("spinner", "create a spinning thread", &spinner)
STATIC_COMMAND("cbuf_tests", "test lib/cbuf", &cbuf_tests)
STATIC_COMMAND_END(tests);

static void tests_init(const struct app_descriptor *app) {
}

APP_START(tests)
.init = tests_init,
.flags = 0,
APP_END

