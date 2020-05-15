/*
 * Copyright (c) 2009 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <stdio.h>
#include <app.h>
#include <kernel/thread.h>

extern const struct app_descriptor __start_apps __WEAK;
extern const struct app_descriptor __stop_apps __WEAK;

static void start_app(const struct app_descriptor *app);

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
        if (app->entry && (app->flags & APP_FLAG_DONT_START_ON_BOOT) == 0) {
            start_app(app);
        }
    }
}

static int app_thread_entry(void *arg) {
    const struct app_descriptor *app = (const struct app_descriptor *)arg;

    app->entry(app, NULL);

    return 0;
}

static void start_app(const struct app_descriptor *app) {
    uint32_t stack_size = (app->flags & APP_FLAG_CUSTOM_STACK_SIZE) ? app->stack_size : DEFAULT_STACK_SIZE;

    printf("starting app %s\n", app->name);
    thread_t *t = thread_create(app->name, &app_thread_entry, (void *)app, DEFAULT_PRIORITY, stack_size);
    thread_detach(t);
    thread_resume(t);
}

