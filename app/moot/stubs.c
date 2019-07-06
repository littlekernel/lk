/*
 * Copyright 2016 Google Inc. All Rights Reserved.
 * Author: gkalsi@google.com (Gurjant Kalsi)
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <app/moot/stubs.h>
#include <lk/compiler.h>
#include <lk/err.h>

// Fail by default. System must override this.
__WEAK status_t moot_mount_default_fs(char **mount_path, char **device_name) {
    *device_name = NULL;
    *mount_path = NULL;
    return ERR_NOT_IMPLEMENTED;
}
