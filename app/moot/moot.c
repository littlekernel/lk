/*
 * Copyright 2016 Google Inc. All Rights Reserved.
 * Author: gkalsi@google.com (Gurjant Kalsi)
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <app/moot/fsboot.h>
#include <app/moot/moot.h>
#include <app/moot/stubs.h>
#include <app/moot/usbboot.h>

#include <app.h>
#include <arch.h>
#include <assert.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <kernel/event.h>
#include <lk/init.h>
#include <stdlib.h>
#include <lk/trace.h>

static void do_boot(void) {
    arch_disable_ints();
    arch_quiesce();
    arch_chain_load((void *)(moot_system_info.sys_base_addr), 0, 0, 0, 0);
}

static void moot_init(const struct app_descriptor *app) {
    // Initialize our boot subsystems.
    init_usb_boot();

}

static void moot_entry(const struct app_descriptor *app, void *args) {
    // Wait a few seconds for the host to try to talk to us over USB.
    printf("attempting usb boot...\n");
    attempt_usb_boot();

    // Check the SPIFlash for an upgrade image.
    printf("attempting fs boot...\n");
    attempt_fs_boot();

    // Boot the main system image.
    printf("proceeding to boot...\n");
    do_boot();
}

APP_START(moot)
.init = moot_init,
.entry = moot_entry,
APP_END
