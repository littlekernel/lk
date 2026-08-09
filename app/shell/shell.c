/*
 * Copyright (c) 2009 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <app.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <kernel/thread.h>
#include <lib/cmdline.h>
#include <lib/console.h>
#include <stdio.h>
#include <stdlib.h>

// Maximum length of a script passed in via the lk.autorun kernel command line variable.
#define AUTORUN_MAX_LEN 512

// Commands run from the shell, notably the unit tests, can be far more stack hungry than
// the default thread stack allows for. Memory constrained targets can override this.
#ifndef SHELL_STACK_SIZE
#define SHELL_STACK_SIZE (DEFAULT_STACK_SIZE)
#endif

// Decode the plus encoded form of an autorun script in place:
//   '++' -> a literal '+'
//   '+'  -> a space
//
// Encoding spaces this way lets a script make it through the layers of host side
// scripting and the bootloader command line without needing to be quoted.
static void decode_pluses(char *str) {
    char *out = str;

    for (const char *in = str; *in != '\0'; in++) {
        if (*in != '+') {
            *out++ = *in;
        } else if (in[1] == '+') {
            *out++ = '+';
            in++;
        } else {
            *out++ = ' ';
        }
    }
    *out = '\0';
}

// If the kernel command line holds an lk.autorun=<script> variable, run the script
// through the console before dropping into the interactive shell.
//
// The value may either be quoted (lk.autorun="ut all; poweroff"), which lib/cmdline
// unquotes for us, or plus encoded (lk.autorun=ut+all;poweroff).
static void run_autorun_script(console_t *con) {
    char *script = malloc(AUTORUN_MAX_LEN);
    if (!script) {
        return;
    }

    status_t err = cmdline_get_string("lk.autorun", script, AUTORUN_MAX_LEN, NULL);
    if (err == NO_ERROR) {
        decode_pluses(script);
        dprintf(INFO, "shell: running autorun script '%s'\n", script);
        console_run_script(con, script);
    } else if (err != ERR_NOT_FOUND) {
        printf("shell: cannot run lk.autorun script (err %d), maximum length is %d\n",
               err, AUTORUN_MAX_LEN - 1);
    }

    free(script);
}

static void shell_entry(const struct app_descriptor *app, void *args) {
    console_t *con = console_create(true);
    if (!con)
        return;

    // Make this the console for the current thread before running anything, since some
    // commands look up the current console instead of being handed one.
    console_set_current(con);

    run_autorun_script(con);

    console_start(con);

    // TODO: destroy console and free resources
}

APP_START(shell)
.entry = shell_entry,
.flags = APP_FLAG_CUSTOM_STACK_SIZE,
.stack_size = SHELL_STACK_SIZE,
APP_END

