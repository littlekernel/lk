/*
 * Copyright (c) 2009 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include "app.h"

#include <stdio.h>
#include <string.h>
#include <lk/err.h>
#include <lk/console_cmd.h>
#include <kernel/thread.h>

extern const struct app_descriptor __start_apps __WEAK;
extern const struct app_descriptor __stop_apps __WEAK;

static void start_app(const struct app_descriptor *app, bool detach);

/* one time setup */
void apps_init(void) {
    const struct app_descriptor *app;

    /* call all the init routines */
    for (app = &__start_apps; app != &__stop_apps; app++) {
        if (app->init)
            app->init(app);
    }

    /* start any that want to start on boot */
    for (app = &__start_apps; app != &__stop_apps; app++) {
        if (app->entry && (app->flags & APP_FLAG_NO_AUTOSTART) == 0) {
            start_app(app, true);
        }
    }
}

static int app_thread_entry(void *arg) {
    const struct app_descriptor *app = (const struct app_descriptor *)arg;

    app->entry(app, NULL);

    return 0;
}

static void start_app(const struct app_descriptor *app, bool detach) {
    uint32_t stack_size = (app->flags & APP_FLAG_CUSTOM_STACK_SIZE) ? app->stack_size : DEFAULT_STACK_SIZE;

    printf("starting app %s\n", app->name);
    thread_t *t = thread_create(app->name, &app_thread_entry, (void *)app, DEFAULT_PRIORITY, stack_size);
    if (detach) {
        thread_detach(t);
        thread_resume(t);
    } else {
        thread_resume(t);
        thread_join(t, NULL, INFINITE_TIME);
    }
}

status_t app_start_by_name(const char *name, bool detached) {
    const struct app_descriptor *app;

    /* find the app and call it */
    for (app = &__start_apps; app != &__stop_apps; app++) {
        if (!strcmp(app->name, name)) {
            start_app(app, detached);
            return NO_ERROR;
        }
    }

    return ERR_NOT_FOUND;
}

static void list_apps(void) {
    const struct app_descriptor *app;

    for (app = &__start_apps; app != &__stop_apps; app++) {
        printf("%s\n", app->name);
    }
}

static int cmd_app(int argc, const console_cmd_args *argv) {
    if (argc == 1) {
usage:
        printf("%s subcommands:\n", argv[0].str);
        printf("%s list         : list apps compiled into the system\n", argv[0].str);
        printf("%s start <name> : run app\n", argv[0].str);
        return ERR_INVALID_ARGS;
    } else if (!strcmp(argv[1].str, "list")) {
        list_apps();
    } else if (!strcmp(argv[1].str, "start")) {
        if (argc <= 2) {
            printf("not enough args\n");
            goto usage;
        }

        app_start_by_name(argv[2].str, false);
    } else {
        printf("unknown subcommand\n");
        goto usage;
    }

    return NO_ERROR;
}

static int cmd_start(int argc, const console_cmd_args *argv) {
    if (argc == 1) {
        printf("not enough args\n");
        return ERR_INVALID_ARGS;
    }

    return app_start_by_name(argv[1].str, false);
}

STATIC_COMMAND_START
STATIC_COMMAND("app", "commands to operate on apps", &cmd_app)
STATIC_COMMAND("start", "shortcut for app start", &cmd_start)
STATIC_COMMAND_END(app);
