/*
 * Copyright (c) 2009-2012 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <stddef.h>
#include <lk/compiler.h>
#include <stdbool.h>
#include <sys/types.h>

__BEGIN_CDECLS

/* app support api */
void apps_init(void); /* one time setup */

/* start an app by name.
 * optionally start detached or wait for it to complete.
 */
status_t app_start_by_name(const char *name, bool detached);

/* app entry point */
struct app_descriptor;
typedef void (*app_init)(const struct app_descriptor *);
typedef void (*app_entry)(const struct app_descriptor *, void *args);

/* app startup flags */
#define APP_FLAG_NO_AUTOSTART 0x1
#define APP_FLAG_CUSTOM_STACK_SIZE 0x2

/* each app needs to define one of these to define its startup conditions */
struct app_descriptor {
    const char *name;
    app_init  init;
    app_entry entry;
    unsigned int flags;
    size_t stack_size;
};

#define APP_START(appname) const struct app_descriptor _app_##appname __USED __ALIGNED(sizeof(void *)) __SECTION("apps") = { .name = #appname,

#define APP_END };

__END_CDECLS

